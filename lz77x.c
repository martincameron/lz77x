
#include "errno.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

/*
	A simple LZ77 implementation with a 64k search window. (c)2018 mumart@gmail.com
	The output consists of token bytes optionally followed by literals.
		0xxxxxxx                   : Offset 0, length X.
		1xxxxxxx yyyyyyyy yyyyyyyy : Offset Y, length X.
	When offset is zero, length is the number of bytes to be copied from the input.
	When offset is positive, length bytes are to be copied from the output.
*/

/*
	Compress count bytes from in to out.
	The output length is returned (out may be null).
*/
int lz77x_enc( char *in, char *out, int count ) {
	int in_idx = 0, out_idx = 0;
	int len, off, max, lit = 0, *hash = calloc( 65536, sizeof( int ) );
	if( hash ) {
		while( in_idx <= count ) {
			len = 1;
			if( in_idx + 2 < count ) {
				off = in_idx - hash[ ( ( in[ in_idx ] & 0xFF ) << 8 ) | ( ( in[ in_idx + 1 ] & 0xFF ) ^ ( in[ in_idx + 2 ] & 0xFF ) ) ];
				if( off > 0 && off < 65536 && in[ in_idx - off ] == in[ in_idx ] ) {
					while( in_idx + len < count && in[ in_idx - off + len ] == in[ in_idx + len ] ) {
						len++;
					}
				}
			}
			if( len > 3 || in_idx == count ) {
				while( lit ) {
					max = lit > 127 ? 127 : lit;
					lit -= max;
					if( out ) {
						out[ out_idx++ ] = max;
						while( max ) {
							out[ out_idx++ ] = in[ in_idx - lit - max ];
							max--;
						}
					} else {
						out_idx += 1 + max;
					}
				}
			}
			if( len > 2 && lit == 0 ) {
				if( len > 127 ) {
					len = 127;
				}
				if( out ) {
					out[ out_idx++ ] = 0x80 | len;
					out[ out_idx++ ] = off >> 8;
					out[ out_idx++ ] = off;
				} else {
					out_idx += 3;
				}
			} else {
				lit += len;
			}
			while( len ) {
				if( in_idx + 2 < count ) {
					hash[ ( ( in[ in_idx ] & 0xFF ) << 8 ) | ( ( in[ in_idx + 1 ] & 0xFF ) ^ ( in[ in_idx + 2 ] & 0xFF ) ) ] = in_idx;
				}
				in_idx++;
				len--;
			}
		}
		free( hash );
	}
	return out_idx;
}

/*
	Decompress count bytes from in to out.
	The output length is returned (out may be null).
*/
int lz77x_dec( char *in, char *out, int count ) {
	int in_idx = 0, out_idx = 0, len, off, end;
	while( in_idx < count ) {
		len = in[ in_idx++ ] & 0xFF;
		if( len > 127 ) {
			len = len & 0x7F;
			off = ( ( in[ in_idx ] & 0xFF ) << 8 ) | ( in[ in_idx + 1 ] & 0xFF );
			in_idx += 2;
			if( out ) {
				end = out_idx + len;
				while( out_idx < end ) {
					out[ out_idx ] = out[ out_idx - off ];
					out_idx++;
				}
			} else {
				out_idx += len;
			}
		} else {
			if( out ) {
				end = out_idx + len;
				while( out_idx < end ) {
					out[ out_idx++ ] = in[ in_idx++ ];
				}
			} else {
				in_idx += len;
				out_idx += len;
			}
		}
	}
	return out_idx;
}

/*
	Load file_name into buffer.
	The file length is returned (buffer may be null).
*/
long lz77x_load_file( char *file_name, char *buffer ) {
	long file_length = -1, bytes_read;
	FILE *input_file = fopen( file_name, "rb" );
	if( input_file != NULL ) {
		if( fseek( input_file, 0L, SEEK_END ) == 0 ) {
			file_length = ftell( input_file );
			if( file_length >= 0 && buffer ) {
				if( fseek( input_file, 0L, SEEK_SET ) == 0 ) {
					bytes_read = fread( buffer, 1, file_length, input_file ); 
					if( bytes_read != file_length ) {
						file_length = -1;
					}
				} else {
					file_length = -1;
				}
			}
		}
		fclose( input_file );
	}
	if( file_length < 0 ) {
		fprintf( stderr, "Unable to load file %s: %s\n", file_name, strerror( errno ) );
	}
	return file_length;
}

/*
	Save count bytes of buffer to file_name.
*/
int lz77x_save_file( char *file_name, char *buffer, int count ) {
	int length = -1;
	FILE *output_file = fopen( file_name, "wb" );
	if( output_file != NULL ) {
		length = fwrite( buffer, 1, count, output_file );
		fclose( output_file );
	}
	if( length < count ) {
		fprintf( stderr, "Unable to save file %s: %s\n", file_name, strerror( errno ) );
	}
	return length;
}

int main( int argc, char **argv ) {
	int arg, decode = 0, input_len, output_len, result = EXIT_FAILURE;
	char *input_file = NULL, *output_file = NULL, *input, *enc, *dec;
	for( arg = 1; arg < argc; arg++ ) {
		/* Parse arguments.*/
		if( strcmp( argv[ arg ], "-decode" ) == 0 ) {
			decode = 1;
		} else if( input_file ) {
			output_file = argv[ arg ];
		} else {
			input_file = argv[ arg ];
		}
	}
	if( input_file ) {
		input_len = lz77x_load_file( input_file, NULL );
		if( input_len >= 0 ) {
			printf( "Input length: %d\n", input_len );
			input = malloc( input_len );
			if( input ) {
				if( decode ) {
					if( lz77x_load_file( input_file, input ) >= 0 ) {
						output_len = lz77x_dec( input, NULL, input_len );
						printf( "Decoded length: %d\n", output_len );
						if( output_file ) {
							dec = malloc( output_len );
							if( dec ) {
								lz77x_dec( input, dec, input_len );
								lz77x_save_file( output_file, dec, output_len );
								free( dec );
							} else {
								fputs( "Out of memory.\n", stderr );
							}
						}
					}
				} else {
					dec = malloc( input_len );
					if( dec ) {
						if( lz77x_load_file( input_file, input ) >= 0 ) {
							output_len = lz77x_enc( input, NULL, input_len );
							printf( "Encoded length: %d\n", output_len );
							enc = malloc( output_len );
							if( enc ) {
								lz77x_enc( input, enc, input_len );
								if( lz77x_dec( enc, dec, output_len ) == input_len ) {
									if( memcmp( input, dec, input_len ) ) {
										fputs( "Decoded data differs from original.\n", stderr );
									} else if( output_file ) {
										lz77x_save_file( output_file, enc, output_len );
									} else {
										puts( "Compression test okay." );
									}
								} else {
									fputs( "Decoded length differs from original.\n", stderr );
								}
								free( enc );
							} else {
								fputs( "Out of memory.\n", stderr );
							}
						}
						free( dec );
					} else {
						fputs( "Out of memory.\n", stderr );
					}
				}
				free( input );
			} else {
				fputs( "Out of memory.\n", stderr );
			}
		}
	} else {
		fprintf( stderr, "Lz77x test program.\nUsage: %s [-decode] input_file [output_file]\n", argv[ 0 ] );
	}
	return result;
}
