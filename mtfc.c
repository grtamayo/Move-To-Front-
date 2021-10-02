/*
Filename:     mtfc.c, move-to-front
Written by:   Gerald R. Tamayo, (October 2, 2021).
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>  /* C99 */
#include <string.h>
#include "gtbitio.c"
#include "ucodes.c"
#include "mtf.c"

#define WBITS       20
#define WSIZE       (1<<WBITS)
#define WMASK       (WSIZE-1)

#define EOF_FLAG    255
#define MTF_MAX_CNT 256

enum {
	/* modes */
	COMPRESS,
	DECOMPRESS,
};

enum {
	/* codes */
	mPPP = 1,
	mVLCODE
};

typedef struct {
	char alg[4];
} file_stamp;

unsigned char win_buf[ WSIZE ];   /* the prediction buffer. */
unsigned char pattern[ WSIZE ];   /* the "look-ahead" buffer. */
int mcode = 0;

void copyright( void );
void   compress( void );
void decompress( void );

void usage( void )
{
	fprintf(stderr, "\n Usage: mtfc c|d infile outfile\n"
		"\n Commands:\n  c = Encoding.\n  d = Decoding.\n"
	);
	copyright();
	exit(0);
}

int main( int argc, char *argv[] )
{
	float ratio = 0.0;
	int mode = -1;
	file_stamp fstamp;
	char *cmd = NULL;
	
	if ( argc != 4 ) usage();
	init_buffer_sizes( (1<<15) );
	
	cmd = argv[argc=1];
	while ( cmd ){
		switch( *cmd ) {
			case 'c': if ( mode == -1 ) mode = COMPRESS; else usage(); cmd++; break;
			case 'd':
				if ( mode == -1 ) mode = DECOMPRESS; else usage();
				if ( *(cmd+1) != 0 ) usage(); cmd++; break;
            /*
			case '1':
				if ( mode == -1 || mode == DECOMPRESS || mcode ) usage();
				mcode = mPPP; cmd++; break;
			case '2':
				if ( mode == -1 || mode == DECOMPRESS || mcode ) usage();
				mcode = mVLCODE; cmd++; break;
            */
			case 0: cmd = NULL; if ( mcode == 0 ) mcode = mPPP; break;
			default : usage();
		}
	}
	argc++;
	
	if ( (gIN=fopen( argv[argc], "rb" )) == NULL ) {
		fprintf(stderr, "\nError opening input file.");
		return 0;
	}
	
	if ( (pOUT=fopen( argv[argc+1], "wb" )) == NULL ) {
		fprintf(stderr, "\nError opening output file.");
		return 0;
	}
	
	/* initialize prediction buffer to all zero (0) values. */
	memset( win_buf, 0, WSIZE );
	alloc_mtf(MTF_MAX_CNT);
	
	if ( mode == COMPRESS ){
		/* Write the FILE STAMP. */
		strcpy( fstamp.alg, "LZP" );
		fstamp.alg[3] = (char) mcode;
		fwrite( &fstamp, sizeof(file_stamp), 1, pOUT );
		init_get_buffer();
		init_put_buffer();
		fprintf(stderr, "\n Encoding ( %s to %s ) ...", 
			argv[argc], argv[argc+1] );
		compress();
	}
	else if ( mode == DECOMPRESS ){
		fread( &fstamp, sizeof(file_stamp), 1, gIN );
		mcode = fstamp.alg[3];
		init_get_buffer();
		init_put_buffer();
		fprintf(stderr, "\n Decoding ( %s to %s ) ...", 
			argv[argc], argv[argc+1] );
		decompress();
	}
	flush_put_buffer();
	
	fprintf(stderr, "done.\n");
	
	free_get_buffer();
	free_put_buffer();
	free_mtf_table();
	if ( gIN ) fclose( gIN );
	if ( pOUT ) fclose( pOUT );
	
	return 0;
}

void copyright( void )
{
	fprintf(stderr, "\n Written by: Gerald R. Tamayo, 2021\n");
}

/* MTF */
void compress( void )
{
	register int c;
	
	while ( (c=get_symbol(8)) != EOF ){
		put_vlcode(mtf(c), 0);
		if ( c == EOF_FLAG ){
			put_ONE(); 
		}
	}
	put_vlcode(mtf(EOF_FLAG), 0);
	put_ZERO();
}

void decompress( void )
{
	register int c;
	
	do {
		c = get_vlcode(0);
		c = get_mtf_c(c);
		if ( c == EOF_FLAG ) {
			if ( get_bit() == 0 ) break;
		}
		pfputc(c);
	} while ( 1 );
}


