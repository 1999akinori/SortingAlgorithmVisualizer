/*******************************************************************************
* This program will extract data from the PS/2 port and use it to the set the initial values of the sorting array
******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "address_map_arm.h"

#define N 10
#define HEIGHT 120
#define xBound 320
#define yBound 240
#define INITIAL 21
#define SPACE 31
#define WIDTH 27

//Global Variables
volatile int pixel_buffer_start;
int numbers[N];

//Forward Declaration
void HEX_PS2(char, char, char);
void initialize();
void clear_screen();
void wait_for_vsync();
void plot_pixel(int x, int y, short int line_color);
void draw_rectangle(int xPos, int height, short int box_color);
void draw_array();

short color[10] = {0xFFFF, 0xF800, 0x07E0, 0x001F, 0x3478, 0xEAFF, 0x07FF, 0xF81F, 0xFFE0, 0x9909}; //List of colors to choose from

int main(void) {
    // Declare the pointer to the PS/2 port and pixel buffer controller
    volatile int * PS2_ptr = (int *)PS2_BASE;
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the 
                                        // back buffer and swap using the wai_for_sync

    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync(); // Setting the S flag to 1 swaps

    /* initialize a pointer to the pixel buffer, 0xC8000000 */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to start of SDRAM memory, 0xC0000000 */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
    initialize();

    int PS2_data, RVALID;
    char byte1 = 0, byte2 = 0, byte3 = 0; // Stores the data from the key




    // while (1) {
    //     PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port
    //     RVALID = PS2_data & 0x8000; // extract the RVALID field
    //     if (RVALID) {
    //         /* shift the next data byte into the display */
    //         // byte1 byte2 byte3
    //         byte1 = byte2;
    //         byte2 = byte3;
    //         byte3 = PS2_data & 0xFF; //New data
    //         HEX_PS2(byte1, byte2, byte3);
    //         if(byte2 == 0xF0){ // No key is being pressed

    //         }else{ // some key is being pressed

    //         }

    //     }
    //     clear_screen();
    //     draw_array();
    //     wait_for_vsync(); // swap front and back buffers on VGA vertical sync
    //     pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    // }
}




/****************************************************************************************
* Subroutine to show a string of HEX data on the HEX displays
****************************************************************************************/
void HEX_PS2(char b1, char b2, char b3) {
    volatile int * HEX3_HEX0_ptr = (int *)HEX3_HEX0_BASE;
    volatile int * HEX5_HEX4_ptr = (int *)HEX5_HEX4_BASE;
    /* SEVEN_SEGMENT_DECODE_TABLE gives the on/off settings for all segments in
    * a single 7-seg display in the DE1-SoC Computer, for the hex digits 0 - F
    */
    unsigned char seven_seg_decode_table[] = {
        0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
        0x7F, 0x67, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};
    unsigned char hex_segs[] = {0, 0, 0, 0, 0, 0, 0, 0};
    unsigned int shift_buffer, nibble;
    unsigned char code;
    int i;
    shift_buffer = (b1 << 16) | (b2 << 8) | b3;
    for (i = 0; i < 6; ++i) {
        nibble = shift_buffer & 0x0000000F; // character is in rightmost nibble
        code = seven_seg_decode_table[nibble];
        hex_segs[i] = code;
    shift_buffer = shift_buffer >> 4;
    }
    /* drive the hex displays */
    *(HEX3_HEX0_ptr) = *(int *)(hex_segs);
    *(HEX5_HEX4_ptr) = *(int *)(hex_segs + 4);
}

void initialize(){
    for(int i=0; i < N; i++){
        numbers[i] = rand()%HEIGHT + 1;
    }
}


void clear_screen(){
    for(int x = 0; x < xBound; x++){
        for(int y = 0; y < yBound; y++){
            plot_pixel(x, y, 0x0000);
        }
    }
}

void plot_pixel(int x, int y, short int line_color){
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

void wait_for_vsync(){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    register int status;

    *pixel_ctrl_ptr = 1; //Set the S bit to 1

    //Wait til the S bit turns into 0 meaning the last pixel reached
    status = *(pixel_ctrl_ptr + 3);
    while((status & 0x01) != 0){
        status = *(pixel_ctrl_ptr + 3);
    }
}


void draw_array(){
    for(int i=0; i < N; i++){
        draw_rectangle(INITIAL + SPACE*i, numbers[i], color[1]); 
    }
}

void draw_rectangle(int xPos, int height, short int box_color){
    
    for (int i = xPos - 13 ; i <= xPos + 13; i++){
        for(int j = 239 - height; j <= 239; j++){
            plot_pixel(i, j, box_color);
        }
    }

}