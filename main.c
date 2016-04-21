#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

#include <stdio.h>
/* include the background image we are using */
#include "realHeli.h"

/* include the sprite image we are using */
#include "realCopter.h"



/* the tile mode flags needed for display control register */
#define MODE0 0x00
#define MODE1 0x01
#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200

/* flags to set sprite handling in display control register */
#define SPRITE_MAP_2D 0x0
#define SPRITE_MAP_1D 0x40
#define SPRITE_ENABLE 0x1000


/* the control registers for the four tile layers */
volatile unsigned short* bg0_control = (volatile unsigned short*) 0x4000008;
volatile unsigned short* bg1_control = (volatile unsigned short*) 0x400000a;
/* palette is always 256 colors */
#define PALETTE_SIZE 256

/* there are 128 sprites on the GBA */
#define NUM_SPRITES 128

/* the display control pointer points to the gba graphics register */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* the memory location which controls sprite attributes */
volatile unsigned short* sprite_attribute_memory = (volatile unsigned short*) 0x7000000;

/* the memory location which stores sprite image data */
volatile unsigned short* sprite_image_memory = (volatile unsigned short*) 0x6010000;

/* the address of the color palettes used for backgrounds and sprites */
volatile unsigned short* bg_palette = (volatile unsigned short*) 0x5000000;
volatile unsigned short* sprite_palette = (volatile unsigned short*) 0x5000200;

/* the button register holds the bits which indicate whether each button has
 * been pressed - this has got to be volatile as well
 */
volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

/* scrolling registers for backgrounds */
volatile short* bg0_x_scroll = (unsigned short*) 0x4000010;
volatile short* bg0_y_scroll = (unsigned short*) 0x4000012;

/* the bit positions indicate each button - the first bit is for A, second for
 * B, and so on, each constant below can be ANDED into the register to get the
 * status of any one button */
#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

/* the scanline counter is a memory cell which is updated to indicate how
 * much of the screen has been drawn */
volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

/* wait for the screen to be fully drawn so we can do something during vblank */
void wait_vblank( ) {
    /* wait until all 160 lines have been updated */
    while (*scanline_counter < 160) { }
}

/* this function checks whether a particular button has been pressed */
unsigned char button_pressed(unsigned short button) {
    /* and the button register with the button constant we want */
    unsigned short pressed = *buttons & button;

    /* if this value is zero, then it's not pressed */
    if (pressed == 0) {
        return 1;
    } else {
        return 0;
    }
}

/* return a pointer to one of the 4 character blocks (0-3) */
volatile unsigned short* char_block(unsigned long block) {
    /* they are each 16K big */
    return (volatile unsigned short*) (0x6000000 + (block * 0x4000));
}

/* return a pointer to one of the 32 screen blocks (0-31) */
volatile unsigned short* screen_block(unsigned long block) {
    /* they are each 2K big */
    return (volatile unsigned short*) (0x6000000 + (block * 0x800));

    
}

/* flag for turning on DMA */
#define DMA_ENABLE 0x80000000

/* flags for the sizes to transfer, 16 or 32 bits */
#define DMA_16 0x00000000
#define DMA_32 0x04000000

/* pointer to the DMA source location */
volatile unsigned int* dma_source = (volatile unsigned int*) 0x40000D4;

/* pointer to the DMA destination location */
volatile unsigned int* dma_destination = (volatile unsigned int*) 0x40000D8;

/* pointer to the DMA count/control */
volatile unsigned int* dma_count = (volatile unsigned int*) 0x40000DC;

/* copy data using DMA */
void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount) {
    *dma_source = (unsigned int) source;
    *dma_destination = (unsigned int) dest;
    *dma_count = amount | DMA_16 | DMA_ENABLE;
}

/* function to setup background 0 for this program */
void setup_background() {

    /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) bg_palette, (unsigned short*) realHeli_palette, PALETTE_SIZE);

    /* load the image into char block 0 */
    memcpy16_dma((unsigned short*) char_block(0), (unsigned short*) realHeli_data, (realHeli_width * realHeli_height) / 2);

    /* set all control the bits in this register */
    *bg0_control = 2 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (16 << 8) |       /* the screen block the tile data is stored in */
        (1 << 13) |       /* wrapping flag */
        (0 << 14);        /* bg size, 0 is 256x256 */

    *bg1_control = 1 |
        (0 << 2)  |
        (0 << 6)  |
        (1 << 7)  |
        (24 << 8) |
        (1 << 13) |
        (0 << 14);


    unsigned short* background =  screen_block(16);
        for(int i = 0; i < 1024; i++)
        {
            background[i] = i;
        }

       background = screen_block(24);
       for(int i = 0; i < 32 * 32; i++){
           background[i] = 0;
       }
}
/* just kill time */
void delay(unsigned int amount) {
    for (int i = 0; i < amount * 10; i++);
}

/* a sprite is a moveable image on the screen */
struct Sprite {
    unsigned short attribute0;
    unsigned short attribute1;
    unsigned short attribute2;
    unsigned short attribute3;
};

/* array of all the sprites available on the GBA */
struct Sprite sprites[NUM_SPRITES];
int next_sprite_index = 0;

/* the different sizes of sprites which are possible */
enum SpriteSize {
    SIZE_8_8,
    SIZE_16_16,
    SIZE_32_32,
    SIZE_64_64,
    SIZE_16_8,
    SIZE_32_8,
    SIZE_32_16,
    SIZE_64_32,
    SIZE_8_16,
    SIZE_8_32,
    SIZE_16_32,
    SIZE_32_64
};

/* function to initialize a sprite with its properties, and return a pointer */
struct Sprite* sprite_init(int x, int y, enum SpriteSize size,
    int horizontal_flip, int vertical_flip, int tile_index, int priority) {

    /* grab the next index */
    int index = next_sprite_index++;

    /* setup the bits used for each shape/size possible */
    int size_bits, shape_bits;
    switch (size) {
        case SIZE_8_8:   size_bits = 0; shape_bits = 0; break;
        case SIZE_16_16: size_bits = 1; shape_bits = 0; break;
        case SIZE_32_32: size_bits = 2; shape_bits = 0; break;
        case SIZE_64_64: size_bits = 3; shape_bits = 0; break;
        case SIZE_16_8:  size_bits = 0; shape_bits = 1; break;
        case SIZE_32_8:  size_bits = 1; shape_bits = 1; break;
        case SIZE_32_16: size_bits = 2; shape_bits = 1; break;
        case SIZE_64_32: size_bits = 3; shape_bits = 1; break;
        case SIZE_8_16:  size_bits = 0; shape_bits = 2; break;
        case SIZE_8_32:  size_bits = 1; shape_bits = 2; break;
        case SIZE_16_32: size_bits = 2; shape_bits = 2; break;
        case SIZE_32_64: size_bits = 3; shape_bits = 2; break;
    }
   
    int h = horizontal_flip ? 1 : 0;
    int v = vertical_flip ? 1 : 0;

    /* set up the first attribute */
    sprites[index].attribute0 = y |             /* y coordinate */
                            (0 << 8) |          /*rendering mode */                                     (0 << 10) |         /* gfx mode */                                          (0 << 12) |         /* mosaic */
                            (1 << 13) |         /* color mode 0:16, 1:256 */                            (shape_bits << 14); /* shape */
    /* set up the second attribute */
    sprites[index].attribute1 = x |             /* x coordinate */
                            (0 << 9) |          /* affine flag */
                            (h << 12) |         /* horizontal flip flag */
                            (v << 13) |         /* vertical flip flag */
                            (size_bits << 14);  /* size */
    /* setup the third attribute */
    sprites[index].attribute2 = tile_index |   // tile index */
                            (priority << 10) | // priority */
                            (0 << 12);         // palette bank (only 16 color)*/
    /* return pointer to this sprite */
    return &sprites[index];
}

/* update all of the spries on the screen */
void sprite_update_all() {
    /* copy them all over */
    memcpy16_dma((unsigned short*) sprite_attribute_memory, (unsigned short*) sprites, NUM_SPRITES * 4);
}

/* setup all sprites */
void sprite_clear() {
    /* clear the index counter */
    next_sprite_index = 0;

    /* move all sprites offscreen to hide them */
    for(int i = 0; i < NUM_SPRITES; i++) {
        sprites[i].attribute0 = SCREEN_HEIGHT;
        sprites[i].attribute1 = SCREEN_WIDTH;
        }
}

/* set a sprite postion */
void sprite_position(struct Sprite* sprite, int x, int y) {
    /* clear out the y coordinate */
    sprite->attribute0 &= 0xff00;

    /* set the new y coordinate */
    sprite->attribute0 |= (y & 0xff);

    /* clear out the x coordinate */
    sprite->attribute1 &= 0xfe00;

    /* set the new x coordinate */
    sprite->attribute1 |= (x & 0x1ff);
}

/* move a sprite in a direction */
void sprite_move(struct Sprite* sprite, int dx, int dy) {
    /* get the current y coordinate */
    int y = sprite->attribute0 & 0xff;

    /* get the current x coordinate */
    int x = sprite->attribute1 & 0x1ff;

    /* move to the new location */
    sprite_position(sprite, x + dx, y + dy);
}

/* change the vertical flip flag */
void sprite_set_vertical_flip(struct Sprite* sprite, int vertical_flip) {
    if (vertical_flip) {
        /* set the bit */
        sprite->attribute1 |= 0x2000;
    } else {
        /* clear the bit */
        sprite->attribute1 &= 0xdfff;
    }
}

/* change the vertical flip flag */
void sprite_set_horizontal_flip(struct Sprite* sprite, int horizontal_flip) {
    if (horizontal_flip) {
        /* set the bit */
        sprite->attribute1 |= 0x1000;
    } else {
        /* clear the bit */
        sprite->attribute1 &= 0xefff;
    }
}

/* change the tile offset of a sprite */
void sprite_set_offset(struct Sprite* sprite, int offset) {
    /* clear the old offset */
    sprite->attribute2 &= 0xfc00;

    /* apply the new one */
    sprite->attribute2 |= (offset & 0x03ff);
}

/* setup the sprite image and palette */
void setup_sprite_image() {
    /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) sprite_palette, (unsigned short*) realCopter_palette, PALETTE_SIZE);
      
    /* load the image into char block 0 */
    memcpy16_dma((unsigned short*) sprite_image_memory, (unsigned short*) realCopter_data, (realCopter_width * realCopter_height) / 2);
}
/* a struct for the koopa's logic and behavior */
struct Wall {
    /* the actual sprite attribute info */
    struct Sprite* sprite;

    /* the x and y postion */
    int x, y;
	int origx, origy;

    /* which frame of the animation he is on */
    int frame;

    /* the number of frames to wait before flipping */

    /* the animation counter counts how many frames until we flip */
    
    /* whether the wall is exploding or not */
    int explode;
};

struct Copter {
    struct Sprite* sprite;
    int x, y;
    int frame;
    int move;
    int border;
};

void copter_init(struct Copter* copter) {
    copter->x = 30;
    copter->y = 120;
    copter->border = 18;
    copter->frame = 0;
    copter->move = 0;
    copter->sprite = sprite_init(copter->x, copter->y, SIZE_16_16, 0, 0, copter->frame, 0);
}

/* initialize the koopa */ //added x and y so function can be reused for multiple walls.
void wall_init(struct Wall* wall, int x, int y) {
     wall->x = x;//80
     wall->y = y;//70
	 wall->origx = x;
	 wall->origy = y;
     wall->frame = 8;
     wall->explode = 0;
     wall->sprite = sprite_init(wall->x, wall->y, SIZE_16_16, 0, 0, wall->frame, 0);
     //wall->sprite = sprite_init(wall->x + 60, wall->y -30, SIZE_16_16, 0, 0, wall->frame, 0);
     //wall->sprite = sprite_init(wall->x + 120, wall->y +30, SIZE_16_16, 0, 0, wall->frame, 0);
}

int wall_left(struct Wall* wal){
	//wal->move = 1;
	if(wal->x<0){
		//delete wall somehow?
		wal->x=wal->origx;
		//or move it back to the right side of the screen in a infinite loop like thing?
		return 1;
	}
	else{
		wal->x--;
		return 0;
	}
}


int copter_up(struct Copter* copter){
    copter->move = 1;

    //if(copter->y > (SCREEN_HEIGHT - 40 - copter->border)) {
	if(copter->y <  0+20){
		copter->move=0;
        return 1;
    } else{
        copter->y-=2;
        return 0;
    }
}

int copter_fall(struct Copter* copter){
	copter->move =1;
	if(copter->y > (SCREEN_HEIGHT -20 - copter->border)){
		copter->move=0;
		return 1;
	} else{
		copter->y++;
		return 0;
	}
	/*
    copter->move = 0;
    copter->frame = 0;
    sprite_set_offset(copter->sprite, copter->frame);
	*/
}

void set_text(char* str, int row, int col){
    int index = row * 32 + col;
    int missing = 32;
    volatile unsigned short* ptr = screen_block(24);
    while (*str) {
        ptr[index] = *str - missing;
        index ++;
        str ++;
    }
}


unsigned short tile_lookup(int x, int y, int xscroll, int yscroll, const unsigned short* tilemap, int tilemap_w, int tilemap_h) {
    x += xscroll;
    y += yscroll;

    x >>= 3;
    y >>= 3;

    while (x >= tilemap_w){
        x -= tilemap_w;
    }
    while (y >= tilemap_h){
        y -= tilemap_h;
    }
    while (x < 0) {
        y += tilemap_h;
    }
    while (y < 0) {
        y += tilemap_h;
    }

    int index = y * tilemap_w +x;

    return tilemap[index];
}
//copter updatea
void copter_update(struct Copter *cop){
	sprite_position(cop->sprite, cop->x, cop->y);
}

void wall_update(struct Wall *wal){
	sprite_position(wal->sprite, wal->x, wal->y);
}


void uppercase(char* s);
/* update the wall */
int main( ) {
   /* we set the mode to mode 0 with bg0 on */
   *display_control = MODE0 | BG0_ENABLE | BG1_ENABLE | SPRITE_ENABLE | SPRITE_MAP_1D;

   /* setup the background 0 */
   setup_background();

   /* setup the sprite image data */
   setup_sprite_image();

   /* clear all the sprites on screen now */
   sprite_clear();

   char msg [32] = "Helicopter";
   uppercase(msg);
   set_text(msg, 50, 50);

   /* create the koopa */
   /*struct Wall wallA;
   struct Wall wallB;
   struct Wall wally; //sorry I really wanted one to be named wall-ie */ //yes, I did just comment my comment. sue me. 
   struct Wall walls[3];
	//sorry I'm a comment everything kinda guy
   wall_init(&walls[0],240,40);
   wall_init(&walls[1],320,70);
   wall_init(&walls[2],360,100);
  /* wall_init(&wallA,240,40);
   wall_init(&wallB,320,70);
   wall_init(&wally,360,100);*/

   struct Copter copter;
   copter_init(&copter);

   /* set initial scroll to 0 */
   int xscroll = 0;
   int yscroll = 0;

   /* loop forever */
   while (1) {
        /* update the wall */
        /*wall_update(&wallA);
		wall_update(&wallB);
		wall_update(&wally);*/
	   	int i;
		for(i=0;i<3;i++){
			wall_update(&walls[i]);
		}
		
		for(i=0;i<3;i++){
			wall_left(&walls[i]);
		}
		/*wall_left(&wallA);
		wall_left(&wallB);
		wall_left(&wally);*/
		//If all my commented code bothers you please delete it. 

        copter_update(&copter);
        if(button_pressed(BUTTON_A)){ //reset button...***** REMOVE LATER ******
			return 0;
		}
		else if(button_pressed(BUTTON_UP)) {
        	copter_up(&copter);
			xscroll++;
        }else{
            copter_fall(&copter);
			xscroll++;
        }
		//check collision:
		for(i=0;i<3;i++){//walls are 8 pixels long right?    Walls are 3 pixels wide by 8 pixels height
			//check x
			//if(walls[i].x == (copter.x+copter.border)){
			if((walls[i].x<= (copter.x+copter.border)) && (walls[i].x>=copter.x)){
				//check y
				if((copter.y >= walls[i].y) && (copter.y <= walls[i].y+8)){
						//collition!
						return 0;
				}
			}
		}

		//copter moves up and down decent. starts choppy, but get smoother the longer the game runs
        /* wait for vblank before scrolling and moving sprites */
        wait_vblank();
       // *bg0_x_scroll = xscroll;
        sprite_update_all();

        /* delay some */
        delay(300);
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
