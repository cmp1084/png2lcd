/****************************************************************************
 * p n g 2 l c d
 *
 * Copyright (c) 2011 Marcus Jansson,
 * <mjansson256@yahoo.se>
 *
 * This is a hack!
 * Feed me with .png file
 * Limited error handling.
 *
 ****************************************************************************/

/* HISTORY
 * --------
 *
 * 2011-06-09 v0.51
 * Fixed the name of the produced .h data struct to underscore the /-char
 * if a filepath is given as filename.
 * Fixed #define for PNG_WIDTH and HEIGHT.
 * Wrote TODO-list and how to report bugs, see TODO.txt and BUGREPORT.txt
 *
 * 2011-05-28 v0.5
 * First version. Basic png2lcd .h data conversion
 */

/** \def Height of the PNG picture to be converted */
#define PNG_HEIGHT 16
/** \def Width of the PNG picture to be converted */
#define PNG_WIDTH 20

/** \def Version number */
#define VERSION 0.51

//Arrange the CGRAM chars on the LCD like this:
//
//+--------------------+
//| 0  2  4  6         |
//| 1  3  5  7         |
//+--------------------+

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

#ifndef png_jmpbuf
	#define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

enum errorcode {
	ERROR = -2,
    OK = 0,
};

//Invert a byte, MSB->LSB etc.
unsigned char invertchar(unsigned char byte)
{
	unsigned char retval = 0, i = 0;
	//Invert 8 bits
	while(i++ < 8) {
		retval <<= 1;
		retval |= (byte & 1);
		byte >>= 1;
	}
	return retval;
}

/** \brief Convert a normal filename to its underscored variant
 *
 * The file name e.g. thatdir/thisfile.png) is converted to something
 * suitable for usage in .h file code, like thatdir_thisfile_png
 */
void dot2underscore(char * filename, char * underscored_name)
{
	int i = 0;
	while(filename[i] != '\0') {
		if(filename[i] == '.' ||
			filename[i] == '/') {
			underscored_name[i] = '_';
		}
		else {
			underscored_name[i] = filename[i];
		}
		++i;
	}
	underscored_name[i] = '\0';	//If you dont like this name, you have to manually adjust it
}

/** \brief Read and convert a .png picture to LCD format
 *
 * See the commented code for details
 */
int read_png(char *file_name)
{
   png_structp png_ptr;
   png_infop info_ptr;
   unsigned int sig_read = 0;
   png_uint_32 width, height;

   FILE *fp;

   if ((fp = fopen(file_name, "rb")) == NULL) {
	   printf("Error! Can not open file %s\n", file_name);
      return (ERROR);
   }

   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also supply the
    * the compiler header file version, so that we know if the application
    * was compiled with a compatible version of the library.  REQUIRED
    */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, png_voidp_NULL,
      png_error_ptr_NULL, png_error_ptr_NULL);

   if (png_ptr == NULL)
   {
      fclose(fp);
      printf("Error! Can not close file");
      return (ERROR);
   }

   /* Allocate/initialize the memory for image information.  REQUIRED. */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      fclose(fp);
      png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
      return (ERROR);
   }

   /* Set error handling if you are using the setjmp/longjmp method (this is
    * the normal method of doing things with libpng).  REQUIRED unless you
    * set up your own error handlers in the png_create_read_struct() earlier.
    */
   if (setjmp(png_jmpbuf(png_ptr)))
   {
      /* Free all of the memory associated with the png_ptr and info_ptr */
      png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
      fclose(fp);
      /* If we get here, we had a problem reading the file */
      return (ERROR);
   }

   /* PNG file I/O method 1 */
   /* Set up the input control if you are using standard C streams */
   png_init_io(png_ptr, fp);

   /* If we have already read some of the signature */
   png_set_sig_bytes(png_ptr, sig_read);

   /*
    * If you have enough memory to read in the entire image at once,
    * and you need to specify only transforms that can be controlled
    * with one of the PNG_TRANSFORM_* bits (this presently excludes
    * dithering, filling, setting background, and doing gamma
    * adjustment), then you can read the entire image (including
    * pixels) into the info structure with this call:
    */
   png_read_png(png_ptr, info_ptr,
				//~ PNG_TRANSFORM_IDENTITY,
				PNG_TRANSFORM_INVERT_MONO,
				//~ PNG_TRANSFORM_STRIP_ALPHA |
				//~ PNG_TRANSFORM_PACKING,
				png_voidp_NULL);
   /* At this point you have read the entire image */

/****************************************************************************
 * 	png2lcd conversion
 ****************************************************************************/
	int i, j;

	png_bytep * row_pointers;

	unsigned char bit, byte;
	unsigned int pixel;

	//Here we store the data from the picture, when we have decided if the current pixel is above the THRESHOLD.
	unsigned char * raw8bit;
	int raw8bitcounter = 0;

	height = png_get_image_height(png_ptr, info_ptr);
	width = png_get_image_width(png_ptr, info_ptr);

	if((height != PNG_HEIGHT ) || (width != PNG_WIDTH)) {
		printf("Error! \n .png image must be %i x %i pixels (w x h)\n", PNG_WIDTH, PNG_HEIGHT);
		printf("%s is %i x %i\n", file_name, (int)width, (int)height);
		return ERROR;
	}

	//printf("Converting %i x %i image to Mizar32 LCD data\n\n", (int)width, (int)height);


	raw8bit = malloc(PNG_WIDTH * PNG_HEIGHT * sizeof(char));

	//Get the location of the .png row pointers
	row_pointers = png_get_rows(png_ptr, info_ptr);
	//~ printf("rowbytes = 0x%x \n", (int)png_get_rowbytes(png_ptr, info_ptr));

	for(i = 0; i < height; i++) {
		//~ printf("\n");

		bit = 0;
		byte = 0;

		for(j = 0; j < png_get_rowbytes(png_ptr, info_ptr); j += 3) {
			//Read a RGB pixel, and average the value, if we are above a
			//threshold value, we set this pixel in our data
			pixel = *(row_pointers[i] + j + 0) +
					*(row_pointers[i] + j + 1) +
					*(row_pointers[i] + j + 2);

			//Shift byte to prepare putting another bit in it at MSB.
			byte >>= 1;

			//Setting THRESHOLD is a bit peculiar
			#define PIXELTHRESHOLD (0xdf * 3 / 1)
			if(pixel < PIXELTHRESHOLD) {
				//We got a pixel for our data
				//Put a bit in the MSB.
				byte |= 0x80;
			}

			//Have we converted 8 bits yet?
			if(++bit == 8) {
				//Debug output
				//~ printf("0x%x ", invertchar(byte));
				//Write converted byte to our raw8bit data matrix. We must invert the byte, due to how it was compiled
				raw8bit[raw8bitcounter++] = invertchar(byte);

				//Reset in order to restart with next byte
				bit = byte = 0;
			}
		}
		//Debug output
		//~ printf("0x%x ", invertchar(byte));
		//Write converted byte to our raw8bit data matrix
		raw8bit[raw8bitcounter++] = invertchar(byte);

	}

	//Convert the raw8bit matrix to CGRAM data
	//Use a mask to find the 5-bit patterns
	unsigned int mask = 0xf80000;
	unsigned int cgchar;	//Acts just as a counter
	unsigned int row;		//Which row in the raw8bit matrix are we reading from (which row in the CGRAM char are we writing to?)
	unsigned int b0, b1, b2;	//Values of byte0 byte1 byte3 from the raw8bit matrix
	unsigned int val;
	unsigned int valshift = 0x13;	//How much must the val value be shifted to become a byte? 0x10 bits + 3 bits

	//~ printf("\n5-bit conversion data for Mizar32 LCD:\n\n");
	#define FILENAMEMAXLENGHT 256
	char underscored_name[FILENAMEMAXLENGHT];
	//Convert two CGRAM chars for every loop.
		//Arrange the chars on the LCD like this:
		//+------------+
		//| 0  2  4  6 |
		//| 1  3  5  7 |
		//+------------+
	//Transform the . in file_name to an _underscored_ name
	dot2underscore(file_name, underscored_name);

	printf("\nunsigned char %s[] = {", underscored_name);

	//We got 8 CGRAM chars, but we do two each loop
	for(cgchar = 0; cgchar < (8 - 1); cgchar += 2) {

		printf("\n\t");					//Print the array on screen nicely
		//Do two CGRAM chars each loop here
		for(row = 0; row < 0x10; row++) {
			b0 = raw8bit[3 * row  + 0] & 0xff;
			b1 = raw8bit[3 * row  + 1] & 0xff;
			b2 = raw8bit[3 * row  + 2] & 0xff;

			//Compile a 24 bits value
			val = (b0 << 0x10) | (b1 << 0x08) | (b2 << 4);
			//mask out the 5 bits we want
			val &= mask;
			//and shift them down to a single 5-bit value
			val >>= valshift;
			//...which is printed on the screen
			printf("0x%02x, ", val);	//Print the array on screen nicely
			//TODO: write cgram to file
		}
		mask >>= 5;			//Calc a new mask value to work on next set of CGRAM chars (2-3, 4-5, 6-7)
		valshift -= 5;		//and also a new valshift value.
	}
	printf("\n};\n\n");					//Print the array on screen nicely
	free(raw8bit);



 /***************************************************************************/

   /* Clean up after the read, and free any memory allocated - REQUIRED */
   png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

   /* Close the file */
   fclose(fp);

   /* That's it */
   return (OK);
}

/** \brief Print the version */
void version(void)
{
	printf("PNG2LCD version %0.1f Copyright (c) Marcus Jansson <mjansson256@yahoo.se>\n", VERSION);
	printf("Mizar32 and LCD hardware by SimpleMachines <http://simplemachines.it>\n");
}

/** \brief Print the usage of the program */
void print_usage(char * programname)
{
	version();
	printf("Usage:\n %s [FILENAME]\n\n", programname);
}

void print_bugreport(void)
{
	printf("The FILENAME must be a simple .png, %ix%i pixels.\nIf you have trouble, see BUGREPORT.txt\n\n", PNG_WIDTH, PNG_HEIGHT);
}

void print_help(void)
{
	printf("Options:\n");
	printf("-h, --help        Print this help\n");
	printf("-v, --version     Print the version and exit\n");
}

/****************************************************************************
 * M A I N ( )
 ****************************************************************************/
int main(int argc, char * argv[])
{
	int rc;

	if(argc == 1) {
		print_usage(argv[0]);
		print_bugreport();
		printf("Error! You must supply a filename\n");
		return ERROR;
	}

	if(!strcmp("-h", argv[1]) ||
		!strcmp("--help", argv[1])) {
			print_usage(argv[0]);
			print_help();
			return 0;
		}

	if(!strcmp("-v", argv[1]) ||
		!strcmp("--version", argv[1])) {
			version();
			return 0;
		}

	if(argc > 2) {
		printf("Skipping extra arguments on commandline\n\n");
	}

	version();
	rc = read_png(argv[1]);
	return rc;
}
