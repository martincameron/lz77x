
/*
	A simple LZ77 implementation with a 64k search window. (c)2013 mumart@gmail.com
	The output consists of token bytes optionally followed by literals.
		0xxxxxxx                   : Offset 0, length X.
		1xxxxxxx yyyyyyyy yyyyyyyy : Offset Y, length X.
	When offset is zero, length is the number of bytes to be copied from the input.
	When offset is positive, length bytes are to be copied from the output.
*/
public class LZ77x {
	// Returns output length.
	public static int encode( byte[] input, byte[] output, int inputLen ) {	
		int[] index = new int[ 65536 ];
		int[] chain = new int[ 65536 ];
		int inputIdx = 0, outputIdx = 0, literals = 0;
		while( inputIdx < inputLen ) {
			int matchOffset = 0, matchLength = 1;
			// Indexed search. Requires 512k of memory.
			if( inputIdx + 2 < inputLen ) {
				int key = ( ( input[ inputIdx ] & 0xFF ) << 8 ) ^ ( ( input[ inputIdx + 1 ] & 0xFF ) << 4 ) ^ ( input[ inputIdx + 2 ] & 0xFF );
				int searchIdx = index[ key ] - 1;
				while( ( inputIdx - searchIdx ) < 65536 && searchIdx > 0 ) {
					if( inputIdx + matchLength < inputLen && input[ inputIdx + matchLength ] == input[ searchIdx + matchLength ] ) {
						int len = 0;
						while( inputIdx + len < inputLen && input[ searchIdx + len ] == input[ inputIdx + len ] ) {
							len++;
						}
						if( len > matchLength ) {
							matchOffset = inputIdx - searchIdx;
							matchLength = len;
						}
					}
					searchIdx = chain[ searchIdx & 0xFFFF ] - 1;
				}
				if( matchLength < 4 ) {
					matchOffset = 0;
					matchLength = 1;
				} else if( matchLength > 127 ) {
					matchLength = 127;
				}
				for( int idx = inputIdx, end = inputIdx + matchLength; idx < end; idx++ ) {
					// Update the index for each byte of the input to be encoded.
					key = ( ( input[ idx ] & 0xFF ) << 8 ) ^ ( ( input[ idx + 1 ] & 0xFF ) << 4 ) ^ ( input[ idx + 2 ] & 0xFF );
					chain[ idx & 0xFFFF ] = index[ key ];
					index[ key ] = idx + 1;
				}
			}
			/*
			// Brute-force search, simple but very slow.
			final int MAX_OFFSET = 65535;
			int searchIdx = inputIdx <= MAX_OFFSET ? 0 : inputIdx - MAX_OFFSET;
			while( searchIdx < inputIdx ) {
				int len = 0;
				while( inputIdx + len < inputLen && input[ searchIdx + len ] == input[ inputIdx + len ] ) {
					len++;
				}
				if( len > matchLength && len > 3 ) {
					matchOffset = inputIdx - searchIdx;
					matchLength = len < 128 ? len : 127;
				}
				searchIdx++;
			}
			*/
			if( matchOffset == 0 ) {
				literals += matchLength;
				inputIdx += matchLength;
			}
			if( literals > 0 ) {
				// Flush literals if match found, end of input, or longest encodable run.
				if( matchOffset > 0 || inputIdx == inputLen || literals == 127 ) {
					output[ outputIdx++ ] = ( byte ) literals;
					int literalIdx = inputIdx - literals;
					while( literalIdx < inputIdx ) {
						output[ outputIdx++ ] = input[ literalIdx++ ];
					}
					literals = 0;
				}
			}
			if( matchOffset > 0 ) {
				//System.out.println( "Offset " + matchOffset + " Length " + matchLength );
				output[ outputIdx++ ] = ( byte ) ( 0x80 | matchLength );
				output[ outputIdx++ ] = ( byte ) ( matchOffset >> 8 );
				output[ outputIdx++ ] = ( byte ) matchOffset;
				inputIdx += matchLength;
			}
		}
		return outputIdx;
	}
	
	// Output may be null to calculate uncompressed length.
	public static int decode( byte[] input, byte[] output, int inputLen ) {
		int inputIdx = 0, outputIdx = 0;
		while( inputIdx < inputLen ) {
			int matchOffset = 0;
			int matchLength = input[ inputIdx++ ] & 0xFF;
			if( matchLength > 127 ) {
				matchLength = matchLength & 0x7F;
				matchOffset = input[ inputIdx++ ] & 0xFF;
				matchOffset = ( matchOffset << 8 ) | ( input[ inputIdx++ ] & 0xFF );
			}
			if( output == null ) {
				outputIdx += matchLength;
				if( matchOffset == 0 ) {
					inputIdx += matchLength;
				}
			} else {
				int outputEnd = outputIdx + matchLength;
				if( matchOffset == 0 ) {
					while( outputIdx < outputEnd ) {
						output[ outputIdx++ ] = input[ inputIdx++ ];
					}
				} else {
					while( outputIdx < outputEnd ) {
						output[ outputIdx ] = output[ outputIdx - matchOffset ];
						outputIdx++;
					}
				}
			}
		}
		return outputIdx;
	}
	
	public static void main( String[] args ) throws Exception {
		if( args.length < 1 ) {
			System.out.println( "LZ77x test program." );
			System.err.println( "Usage: java " + LZ77x.class.getName() + " input" );
		} else {
			java.io.InputStream is = new java.io.FileInputStream( args[ 0 ] );
			byte[] input = new byte[ 65536 ];
			int length = 0, count = 0;
			while( count >= 0 ) {
				// Read the entire InputStream into an array.
				count = is.read( input, length, input.length - length );
				if( count > 0 ) {
					length += count;
					if( length >= input.length ) {
						byte[] buf = new byte[ input.length * 2 ];
						System.arraycopy( input, 0, buf, 0, input.length );
						input = buf;
					}
				}
			}
			System.out.println( "Input length: " + length );
			byte[] encoded = new byte[ length * 2 ];
			int encLen = encode( input, encoded, length );
			System.out.println( "Encoded length: " + encLen );
			byte[] decoded = new byte[ length ];
			if( length != decode( encoded, null, encLen ) || length != decode( encoded, decoded, encLen ) ) {
				throw new Exception( "Decoded length differs from original." );
			}
			for( int idx = 0; idx < length; idx++ ) {
				if( decoded[ idx ] != input[ idx ] ) {
					throw new Exception( "Decoded data corrupt at index " + idx );
				}
			}
		}
	}
}
