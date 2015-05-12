#include "16F690.h"
#include "int16Cxx.h"
#pragma config |= 0x00D4

void Round200 ( uns8 i );
void Keccak_f200();
void KeccakHash ();
uns8 getState( uns8 x, uns8 y );
void setState( uns8 x, uns8 y, uns8 a );
uns8 rotate8 ( uns8 a, uns8 i );

char getchar_eedata( char adress );
void putchar_eedata( char data, char adress );

/* round constants */
static const uns8 RC[17] = 
{
	0x01, 0x82, 0x8a, 0x00,
	0x8b, 0x01, 0x81, 0x09,
	0x8a, 0x88, 0x09, 0x0a,
	0x8b, 0x8b, 0x89, 0x03,
	0x02 
};

/* [x, y] */
/* rotation offsets */
static const uns8 RO[25] =
{
/*Y= 0	1	2	3	4 */
	00, 36, 03, 41, 18, /* x = 0 */
	01, 44, 10, 45, 02, /* x = 1 */
	62, 06, 43, 15, 61, /* x = 2 */
	28, 55, 25, 21, 56, /* x = 3 */
	27, 20, 39, 08, 14  /* x = 4 */
};

/* the state is in another rambank because its big */
#pragma rambank 1
uns8 A[25]; /* the state */
/* change back the rambank now */

#pragma rambank 0
uns8 PIN[4]; /* password PIN, 4 digits */
uns8 DIGEST[8]; /* the hash, 64 bits long */ 

void main(void)
{
	uns8 i, data;
	for ( i = 0; i < 4; i ++ )
	{
		data = getchar_eedata( i );
		PIN[i] = data;
	}
	
	KeccakHash ();
	
	for ( i = 0; i < 8; i ++ )
	{
		data = DIGEST[i];
		putchar_eedata( data, i + 4 );
	}
}

void putchar_eedata( char data, char adress )
{
/* Put char in specific EEPROM-adress */
      /* Write EEPROM-data sequence                          */
      EEADR = adress;     /* EEPROM-data adress 0x00 => 0x40 */
      EEPGD = 0;          /* Data, not Program memory        */  
      EEDATA = data;      /* data to be written              */
      WREN = 1;           /* write enable                    */
      EECON2 = 0x55;      /* first Byte in comandsequence    */
      EECON2 = 0xAA;      /* second Byte in comandsequence   */
      WR = 1;             /* write                           */
      while( EEIF == 0) ; /* wait for done (EEIF=1)          */
      WR = 0;
      WREN = 0;           /* write disable - safety first    */
      EEIF = 0;           /* Reset EEIF bit in software      */
      /* End of write EEPROM-data sequence                   */
}


char getchar_eedata( char adress )
{
/* Get char from specific EEPROM-adress */
      /* Start of read EEPROM-data sequence                */
      char temp;
      EEADR = adress;  /* EEPROM-data adress 0x00 => 0x40  */ 
      EEPGD = 0;       /* Data not Program -memory         */      
      RD = 1;          /* Read                             */
      temp = EEDATA;
      RD = 0;
      return temp;     /* data to be read                  */
      /* End of read EEPROM-data sequence                  */  
}

uns8 getState( uns8 x, uns8 y )
{
	return A[ 5 * x + y ];
}

void setState ( uns8 x, uns8 y, uns8 a )
{
	A[ 5 * x + y ] = a;
}

uns8 rotate8 ( uns8 a, uns8 i )
{
	uns8 j;
	for ( j = 0; j < i; j++ )
		a = rr ( a );
	
	return a;
}

void Keccak_f200()
{
	uns8 i;
	for ( i = 0; i < 17; i++ )
		Round200 ( i );
}

void Round200 ( uns8 i )
{
	uns8 B[25];
	uns8 C[5];
	uns8 D[5];
	uns8 x;
	uns8 y;
	uns8 a, b, c, d;
	
	/* theta */
	for ( x = 0; x < 5; x++ )
	{
		a = getState( x, 0 );
		b = getState( x, 1 );
		c = getState( x, 2 );
		d = getState( x, 3 );
		C[x] = a ^ b ^ c ^ d;
	}
	
	for ( x = 0; x < 5; x++ )
	{
		a = C[x - 1];
		b = C[x + 1];
		d = rr ( b );
		D[x] = a ^ d;
	}
	
	for ( x = 0; x <5; x++ )
	{
		for ( y = 0; y < 5; y++ )
		{
			a = getState ( x, y);
			b = a ^ D[x];
			setState ( x, y, b );
		}
	}
	
	/* rho and pi */
	for ( x = 0; x <5; x++ )
	{
		for ( y = 0; y < 5; y++ )
		{
			a = 2*x;
			a = a + 3*y;
			a = a % 5;
			b = getState ( x, y );
			c = RO[5*x + y];
			d = rotate8 ( b, c );
			
			B[y*5 + a] = d;
		}
	}

	/* chi */
	for ( x = 0; x <5; x++ )
	{
		for ( y = 0; y < 5; y++ )
		{
			a = B[x*5 + y];
			d = x + 1;
			b = B[d*5 + y];
			d = d + 1;
			c = B[d*5 + y];
			
			d = a ^ (~b & c);
			setState(x, y, d);
		}
	}
	
	/* iota */
	a = getState ( 0, 0 );
	a = a ^ RC[i];
	setState(0, 0, a);	
}


void KeccakHash ()
{
	uns8 i, j, a, b;
	uns8 padded[8];
	//initialize state
	for ( i = 0; i < 25; i++ )
		A[i] = 0;
		
	//pad message
	for ( i = 0; i < 4; i++ )
	{
		a = PIN[i];
		padded[i] = a;
	}
	
	padded[4] = 0x01;
	padded[5] = 0x00;
	padded[6] = 0x00;
	padded[7] = 0x80;
	
	/* since bitrate = 4 lanes, input is divided in two blocks */
	/* each byte of the block goes into a lane */
	/* ABSORB */
	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 4;  j++ )
		{
			a = i*4 + j;
			a = padded[a];
			setState(0, j, a);
		}
		
		Keccak_f200();
	}
	
	/* SQUEEZE */
	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 4;  j++ )
		{
			a = i*4 + j;
			b = getState(0, j);
			DIGEST[a] = b;
		}
		
		Keccak_f200();
	}
	
}