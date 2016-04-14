// main.c

/* the width and height of the screen */
    #define WIDTH 240
    #define HEIGHT 160

/* the palette always has 256 entries */
    #define PALETTE_SIZE 256

/* include the actual image file data */
    #include "realHeli.h"

/* pointers to the front and back buffers - the front buffer is the start
*  * of the screen array and the back buffer is a pointer to the second half*/
volatile unsigned short* front_buffer = (volatile unsigned short*) 0x6000000;
volatile unsigned short* back_buffer = (volatile unsigned short*)  0x600A000;

/* the address of the color palette used in graphics mode 4 */
volatile unsigned short* palette = (volatile unsigned short*) 0x5000000;

/* the display control pointer points to the gba graphics register */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* these identifiers define different bit positions of the display control */
#define MODE4 0x0004
#define BG2 0x0400

/* this bit indicates whether to display the front or the back buffer
*  * this allows us to refer to bit 4 of the display_control register */
#define SHOW_BACK 0x10;

/* the scanline counter is a memory cell which is updated to indicate how
*  * much of the screen has been drawn */
volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

/* wait for the screen to be fully drawn so we can do something during vblank */
void wait_vblank( ) {
    /* wait until all 160 lines have been updated */
    while (*scanline_counter < 160) { }
}

/* this function takes a video buffer and returns to you the other one */
volatile unsigned short* flip_buffers(volatile unsigned short* buffer) {
    /* if the back buffer is up, return that */
    if(buffer == front_buffer) {
        /* clear back buffer bit and return back buffer pointer */
        *display_control &= ~SHOW_BACK;
        return back_buffer;
    } else {
        /* set back buffer bit and return front buffer */
        *display_control |= SHOW_BACK;
        return front_buffer;
    } 
}

/* write an image into a buffer */
void draw_image(volatile unsigned short* buffer, const unsigned char* data) {
    /* copy the data 16 bits at a time */
    unsigned short* data_wide = (unsigned short*) data;

    for (int i = 0; i < (WIDTH * HEIGHT) / 2; i++) {
        buffer[i] = data_wide[i];

    }
}

/* the main function */
int main( ) { 
    /* we set the mode to mode 4 with bg2 on */
    *display_control = MODE4 | BG2;

    /* load the palette into palette memory */
    for (int i = 0; i < PALETTE_SIZE; i++) {
        palette[i] = realHeli_palette[i];
    }

    /* the buffer we start with */
    volatile unsigned short* buffer = front_buffer;

    /* loop forever */
    while (1) {
        draw_image(buffer, realHeli_data);

        /* wiat for vblank before switching buffers */
        wait_vblank();
        /* swap the buffers */
        buffer = flip_buffers(buffer);
     }

}

/* the game boy advance uses "interrupts" to handle certain situations
* for now we will ignore these */
void interrupt_ignore( ) {
    /* do nothing */
}

/* this table specifies which interrupts we handle which way
* for now, we ignore all of them */
typedef void (*intrp)( );
const intrp IntrTable[13] = {
    interrupt_ignore,   /* V Blank interrupt */
    interrupt_ignore,   /* H Blank interrupt */
    interrupt_ignore,   /* V Counter interrupt */
    interrupt_ignore,   /* Timer 0 interrupt */
    interrupt_ignore,   /* Timer 1 interrupt */
    interrupt_ignore,   /* Timer 2 interrupt */
    interrupt_ignore,   /* Timer 3 interrupt */
    interrupt_ignore,   /* Serial communication interrupt */
    interrupt_ignore,   /* DMA 0 interrupt */
    interrupt_ignore,   /* DMA 1 interrupt */
    interrupt_ignore,   /* DMA 2 interrupt */
    interrupt_ignore,   /* DMA 3 interrupt */
    interrupt_ignore,   /* Key interrupt */
};
