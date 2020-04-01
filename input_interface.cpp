/*******************************************************************************
* This program will extract data from the PS/2 port and use it to the set the initial values of the sorting array
******************************************************************************/
#include <stdlib.h>
#include <stdio.h>

// address_map_arm.h --------------------------------------------------------------------------------------------------------
#define BOARD                 "DE1-SoC"

/* Memory */
#define DDR_BASE              0x00000000
#define DDR_END               0x3FFFFFFF
#define A9_ONCHIP_BASE        0xFFFF0000
#define A9_ONCHIP_END         0xFFFFFFFF
#define SDRAM_BASE            0xC0000000
#define SDRAM_END             0xC3FFFFFF
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_ONCHIP_END       0xC803FFFF
#define FPGA_CHAR_BASE        0xC9000000
#define FPGA_CHAR_END         0xC9001FFF

/* Cyclone V FPGA devices */
#define LEDR_BASE             0xFF200000
#define HEX3_HEX0_BASE        0xFF200020
#define HEX5_HEX4_BASE        0xFF200030
#define SW_BASE               0xFF200040
#define KEY_BASE              0xFF200050
#define JP1_BASE              0xFF200060
#define JP2_BASE              0xFF200070
#define PS2_BASE              0xFF200100
#define PS2_DUAL_BASE         0xFF200108
#define JTAG_UART_BASE        0xFF201000
#define JTAG_UART_2_BASE      0xFF201008
#define IrDA_BASE             0xFF201020
#define TIMER_BASE            0xFF202000
#define AV_CONFIG_BASE        0xFF203000
#define PIXEL_BUF_CTRL_BASE   0xFF203020
#define CHAR_BUF_CTRL_BASE    0xFF203030
#define AUDIO_BASE            0xFF203040
#define VIDEO_IN_BASE         0xFF203060
#define ADC_BASE              0xFF204000

/* Cyclone V HPS devices */
#define HPS_GPIO1_BASE        0xFF709000
#define HPS_TIMER0_BASE       0xFFC08000
#define HPS_TIMER1_BASE       0xFFC09000
#define HPS_TIMER2_BASE       0xFFD00000
#define HPS_TIMER3_BASE       0xFFD01000
#define FPGA_BRIDGE           0xFFD0501C

/* ARM A9 MPCORE devices */
#define   PERIPH_BASE         0xFFFEC000    // base address of peripheral devices
#define   MPCORE_PRIV_TIMER   0xFFFEC600    // PERIPH_BASE + 0x0600

/* Interrupt controller (GIC) CPU interface(s) */
#define MPCORE_GIC_CPUIF      0xFFFEC100    // PERIPH_BASE + 0x100
#define ICCICR                0x00          // offset to CPU interface control reg
#define ICCPMR                0x04          // offset to interrupt priority mask reg
#define ICCIAR                0x0C          // offset to interrupt acknowledge reg
#define ICCEOIR               0x10          // offset to end of interrupt reg
/* Interrupt controller (GIC) distributor interface(s) */
#define MPCORE_GIC_DIST       0xFFFED000    // PERIPH_BASE + 0x1000
#define ICDDCR                0x00          // offset to distributor control reg
#define ICDISER               0x100         // offset to interrupt set-enable regs
#define ICDICER               0x180         // offset to interrupt clear-enable regs
#define ICDIPTR               0x800         // offset to interrupt processor targets regs
#define ICDICFR               0xC00         // offset to interrupt configuration regs
//------------------------------------------------------------------------------------------------------------------------------------------------------

#define N 10
#define HEIGHT 120
#define xBound 320
#define yBound 240
#define INITIAL 21
#define SPACE 31
#define WIDTH 27
#define UP 1
#define DOWN 2
#define RIGHT 3
#define LEFT 4

//Global Variables
volatile int pixel_buffer_start;
int numbers[N];
int current;


//Forward Declaration
void HEX_PS2(char, char, char);
void initialize();
void clear_screen();
void wait_for_vsync();
void plot_pixel(int x, int y, short int line_color);
void draw_rectangle(int xPos, int height, short int box_color);
void draw_array();
void modify_array(int instruction);

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
    int PS2_data, RVALID, RAVAIL;
    char byte1 = 0, byte2 = 0, byte3 = 0; // Stores the data from the key
    *(PS2_ptr) = 0xFF; // reset

    current = 0;

    while (1) {
        PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port
        RVALID = PS2_data & 0x8000; // extract the RVALID field
        RAVAIL = *(PS2_ptr) & 0xFFFF0000; // How many data are in the key
        if (RVALID) {
            /* shift the next data byte into the display */
            // byte1 byte2 byte3
            byte1 = byte2;
            byte2 = byte3;
            byte3 = PS2_data & 0xFF; //New data
            HEX_PS2(byte1, byte2, byte3);
        }
        if(RAVAIL == 0){ // The FIFO is empty and all input have been recieved
            int instruction = 0;
            if((byte1 == 0xE0) & (byte2 == 0xE0)){ //Pressed arrow
                if(byte3 == 0x75) instruction = UP;
                if(byte3 == 0x6B) instruction = LEFT;
                if(byte3 == 0x74) instruction = RIGHT;
                if(byte3 == 0x72) instruction = DOWN;
            }
            modify_array(instruction);
            byte1 = 0; // terminate instructions
            byte2 = 0;
            byte3 = 0;
        }
        clear_screen();
        draw_array();
        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
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
        if(i == current){
            draw_rectangle(INITIAL + SPACE*i, numbers[i], color[2]); 
        }else{
            draw_rectangle(INITIAL + SPACE*i, numbers[i], color[1]); 
        }
    }
}

void draw_rectangle(int xPos, int height, short int box_color){
    
    for (int i = xPos - 13 ; i <= xPos + 13; i++){
        for(int j = 239 - height; j <= 239; j++){
            plot_pixel(i, j, box_color);
        }
    }

}

void modify_array(int instruction){
    if(instruction == UP){
        if(numbers[current] != HEIGHT){
            if((numbers[current] + 10) >= HEIGHT){
                numbers[current] = HEIGHT;
            }else{
                numbers[current] += 10;
            }
        }
    }if(instruction == DOWN){
        if(numbers[current] != 0){
            if((numbers[current] - 10) <= 0){
                numbers[current] = 0;
            }else{
                numbers[current] -= 10;
            }
        }
    }if(instruction == RIGHT){
        if(current != (N - 1)){
            current = current + 1;
        }
    }if(instruction == LEFT){
        if(current != 0){
            current = current - 1;
        }
    }
}