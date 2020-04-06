// Libraries
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include "address_map_arm.h"

// Sorting Related
#define Y_LIMIT 120
#define ANIMATION_SPEED 20
#define MAX_ANIMATION_SPEED 20
#define HIGHLIGHT_COLOR 0xFFE0
// Both
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
volatile int * timer_addr; 
volatile int * pixel_ctrl_ptr;

int numbers[N];
int current;
short color[10] = {0xFFFF, 0xF800, 0x07E0, 0x001F, 0x3478, 0xEAFF, 0x07FF, 0xF81F, 0xFFE0, 0x9909}; //List of colors to choose from

//Forward Declaration
void HEX_PS2(char, char, char);
void initialize();
void clear_screen();
void wait_for_vsync();
void plot_pixel(int x, int y, short int line_color);
void draw_rectangle(int xPos, int yPos, int height, short int box_color);
void draw_array();
void modify_array(int instruction);


void animate_swap(int xPos1, int xPos2, int height1, int height2,  short int box_color1, short int box_color2);
void animate_double_swap(int xPos1, int xPos2, int height1, int height2,  short int box_color1, short int box_color2);
void swap(int* x, int* y);
void bubble_sort(int a[], int size);
void quick_sort(int a[], int low, int high);

void draw_array();

int main(void)
{
    // Declare the pointer to the PS/2 port
    volatile int * PS2_ptr = (int *)PS2_BASE;

    /*Load A9 Timer*/
    timer_addr = (int *) 0xFFFEC600;
    *(timer_addr) = 200000000; //Load a value corresponding to 0.25 seconds
    //*(timer_addr + 2) = 0b10; //Set A bit to 1, to let timer restart after reaching 0

    pixel_ctrl_ptr = (int *)0xFF203020;

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();



    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer


    srand(time(0));
    initialize(); //Initialize array
    /*Clear both buffers before doing any drawing*/
    clear_screen(); //Clear buffer 1

    int PS2_data, RVALID, RAVAIL;
    char byte1 = 0, byte2 = 0, byte3 = 0; // Stores the data from the key
    *(PS2_ptr) = 0xFF; // reset

    current = 0;
    
    //When enter is placed the process it done
    bool enter = false;

    while (!enter) {
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
        if(byte1 == 0x5A | byte2 == 0x5A | byte3 == 0x5A) enter = true;
        if(byte3 == 0xF0){ // The FIFO is empty and all input have been recieved
            int instruction = 0;
            if(byte1 == 0x75) instruction = UP;
            if(byte1 == 0x6B) instruction = LEFT;
            if(byte1 == 0x74) instruction = RIGHT;
            if(byte1 == 0x72) instruction = DOWN;
            modify_array(instruction);
            byte1 = 0; // terminate instructions
            byte2 = 0;
            byte3 = 0;
            clear_screen();
            draw_array();
            wait_for_vsync(); // swap front and back buffers on VGA vertical sync
            pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
        }
    }
    // Prevent highlighting 
    current = -1;

    draw_array();
    //swap buffers
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    clear_screen(); //Clear buffer 2
    draw_array();
    //swap buffers
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer



    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer


    //swap buffers
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer

    //bubble_sort(numbers, N);
    quick_sort(numbers, 0, N-1);
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
            draw_rectangle(INITIAL + SPACE*i,0, numbers[i], color[2]); 
        }else{
            draw_rectangle(INITIAL + SPACE*i,0, numbers[i], color[1]); 
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



/**********************************************************************************************************************
 * Takes in info of rect 1 (should be located on the left) and rect 2 (should be located on the right).
 * The animation is as follows:
 * Rect 1 and 2 will be pushed up
 * Rect 1 will be pushed to the right in place of rect 2, and rect 2 will be pushed left in place of rect 1
 * Rect 1 and 2 will be lowered back down
 * *******************************************************************************************************************/
void animate_double_swap(int xPos1, int xPos2, int height1, int height2,  short int box_color1, short int box_color2){

    //Lifting Rect 1 and 2 up
    int startY = 0;
    int y;
    for (y = startY; y <= Y_LIMIT; y += ANIMATION_SPEED){

        //Erase previous rectangle
        if (y >= 2*ANIMATION_SPEED){
            draw_rectangle(xPos1, y - 2*ANIMATION_SPEED, height1, 0x0000); //Note that previous rectangle on the swapped buffer was two positions behind current position
            draw_rectangle(xPos2, y - 2*ANIMATION_SPEED, height2, 0x0000); //Note that previous rectangle on the swapped buffer was two positions behind current position
        }
        else{
            draw_rectangle(xPos1, startY, height1, 0x0000); 
            draw_rectangle(xPos2, startY, height2, 0x0000); 
        }

        //Draw new rectangle
        draw_rectangle(xPos1, y, height1, HIGHLIGHT_COLOR);
        draw_rectangle(xPos2, y, height2, HIGHLIGHT_COLOR);

        //swap buffers
        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer

        //Update location
        //done using the for loop
    }

    y = y-ANIMATION_SPEED; //store the last position of rect 2
    //Make sure image is aligned in the two buffers
    draw_rectangle(xPos1, y - ANIMATION_SPEED, height1, 0x0000);
    draw_rectangle(xPos2, y - ANIMATION_SPEED, height2, 0x0000);
    draw_rectangle(xPos1, Y_LIMIT, height1, HIGHLIGHT_COLOR);
    draw_rectangle(xPos2, Y_LIMIT, height2, HIGHLIGHT_COLOR);
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    draw_rectangle(xPos1, y, height1, 0x0000);
    draw_rectangle(xPos2, y, height2, 0x0000);
    y = Y_LIMIT;
    draw_rectangle(xPos1, y, height1, HIGHLIGHT_COLOR);
    draw_rectangle(xPos2, y, height2, HIGHLIGHT_COLOR);


    int x1, x2;
    //Move Rect 2 to xPos 1 and rect 1 to xPos 2
    for (x2 = xPos2, x1 = xPos1; x2 >= xPos1 && x1 <= xPos2; x2 -= ANIMATION_SPEED, x1 += ANIMATION_SPEED){

        //Erase previous rectangles
        if (x2 <= xPos2 - 2*ANIMATION_SPEED){
            draw_rectangle(x2 + 2*ANIMATION_SPEED, y, height2, 0x0000); //Note that previous rectangle on the swapped buffer was two positions behind current position        
        }
        else{
            draw_rectangle(xPos2, y, height2, 0x0000);
        }

        if (x1 >= xPos1 + 2*ANIMATION_SPEED){
            draw_rectangle(x1 - 2*ANIMATION_SPEED, y, height1, 0x0000); //Note that previous rectangle on the swapped buffer was two positions behind current position        
        }
        else{
            draw_rectangle(xPos1, y, height1, 0x0000);
        }
        

        //Draw new rectangle
        draw_rectangle(x2, y, height2, HIGHLIGHT_COLOR);
        draw_rectangle(x1, y, height1, HIGHLIGHT_COLOR);

        //swap buffers
        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer

        //Update location
        //done using the for loop

    }
    x2 = x2 + ANIMATION_SPEED;
    x1 = x1 - ANIMATION_SPEED;
    draw_rectangle(x2 + ANIMATION_SPEED, y, height2, 0x0000);
    draw_rectangle(x1 - ANIMATION_SPEED, y, height1, 0x0000);
    draw_rectangle(xPos1, y, height2, HIGHLIGHT_COLOR);
    draw_rectangle(xPos2, y, height1, HIGHLIGHT_COLOR);
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    draw_rectangle(x2, y, height2, 0x0000);
    draw_rectangle(x1, y, height1, 0x0000);
    x2 = xPos1;
    x1 = xPos2;
    draw_rectangle(x2, y, height2, HIGHLIGHT_COLOR);
    draw_rectangle(x1, y, height1, HIGHLIGHT_COLOR);

    //Move Rect 1 & 2 back down
    for (y = Y_LIMIT; y >= 0; y -= ANIMATION_SPEED){

        //Erase previous rectangle
        if (y <= Y_LIMIT - 2*ANIMATION_SPEED){
            draw_rectangle(x2, y + 2*ANIMATION_SPEED, height2, 0x0000); //Note that previous rectangle on the swapped buffer was two positions behind current position        
            draw_rectangle(x1, y + 2*ANIMATION_SPEED, height1, 0x0000); //Note that previous rectangle on the swapped buffer was two positions behind current position        
        }

        else{
            draw_rectangle(x2, Y_LIMIT, height2, 0x0000);
            draw_rectangle(x1, Y_LIMIT, height1, 0x0000);
        }
        

        //Draw new rectangle
        draw_rectangle(x2, y, height2, HIGHLIGHT_COLOR);
        draw_rectangle(x1, y, height1, HIGHLIGHT_COLOR);

        //swap buffers
        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer

        //Update location
        //done using the for loop

    }
    y = y + ANIMATION_SPEED;
    draw_rectangle(x2, y + ANIMATION_SPEED, height2, 0x0000);
    draw_rectangle(x1, y + ANIMATION_SPEED, height1, 0x0000);
    draw_rectangle(x2, 0, height2, HIGHLIGHT_COLOR);
    draw_rectangle(x1, 0, height1, HIGHLIGHT_COLOR);
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    draw_rectangle(x2, y, height2, 0x0000);
    draw_rectangle(x1, y, height1, 0x0000);
    y = 0;
    draw_rectangle(x2, y, height2, HIGHLIGHT_COLOR);
    draw_rectangle(x1, y, height1, HIGHLIGHT_COLOR);


}


/* This function takes last element as pivot, places 
   the pivot element at its correct position in sorted 
    array, and places all smaller (smaller than pivot) 
   to left of pivot and all greater elements to right 
   of pivot */
int partition (int a[], int low, int high) 
{ 
    int pivot = a[high];    // pivot 
    int i = (low - 1);  // Index of smaller element 
  
    for (int j = low; j <= high- 1; j++) 
    { 
        // If current element is smaller than the pivot 
        if (a[j] < pivot) 
        { 
            i++;    // increment index of smaller element 
            //Animate the swap of the rectangles
            int xPos1 = INITIAL + SPACE*i;
            int xPos2 = INITIAL + SPACE*j;
            animate_double_swap(xPos1,xPos2,a[i], a[j] ,color[1], color[1]);
            swap(&a[i], &a[j]); 
        } 
    } 

    //Animate the swap of the rectangles
    int xPos1 = INITIAL + SPACE*(i+1);
    int xPos2 = INITIAL + SPACE*(high);
    animate_double_swap(xPos1,xPos2,a[i + 1], a[high] ,color[1], color[1]);
    swap(&a[i + 1], &a[high]); 
    return (i + 1); 
} 
  

void quick_sort(int a[], int low, int high) 
{ 
    if (low < high) 
    { 
        /* pi is partitioning index, arr[p] is now 
           at right place */
        int pi = partition(a, low, high); 
  
        // Separately sort elements before 
        // partition and after partition 
        quick_sort(a, low, pi - 1); 
        quick_sort(a, pi + 1, high); 
    } 
}


void swap(int* x, int* y){

    int temp = *x; //Store old value stored at x
    *x = *y; //Put value at y into x
    *y = temp; //Value at y will be old value at x

}

// A function to implement bubble sort 
void bubble_sort(int a[], int size) { 

    
   for (int i = 0; i < size-1; i++){       

       for (int j = 0; j < size-i-1; j++){
            int xPos1 = INITIAL + SPACE*j;
            int xPos2 = INITIAL + SPACE*(j+1);

            //Highlight the rectangles
            draw_rectangle(xPos1,0, a[j], HIGHLIGHT_COLOR);
            draw_rectangle(xPos2,0, a[j+1], HIGHLIGHT_COLOR);
            wait_for_vsync();
            pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
            draw_rectangle(xPos1,0, a[j], HIGHLIGHT_COLOR);
            draw_rectangle(xPos2,0, a[j+1], HIGHLIGHT_COLOR);

            *(timer_addr) = 50000000 * (MAX_ANIMATION_SPEED / ANIMATION_SPEED); 
            *(timer_addr + 2) = 0b1; //Enable timer
            while (*(timer_addr + 3) != 1){
                //wait
            }
            *(timer_addr + 3) = 1; //Reset F bit to 0;
            if (a[j] > a[j+1]) {

                //Animate the swap of the rectangles
                animate_double_swap(xPos1,xPos2,a[j], a[j+1] ,color[1], color[1]);

                //Unhighlight the rectangles
                draw_rectangle(xPos1,0, a[j+1], color[1]);
                draw_rectangle(xPos2,0, a[j], color[1]);
                wait_for_vsync();
                pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
                draw_rectangle(xPos1,0, a[j+1], color[1]);
                draw_rectangle(xPos2,0, a[j], color[1]);

                //Swap the array elements 
                swap(&a[j], &a[j+1]); 

            }
            else{
                //Unhighlight the rectangles

                draw_rectangle(xPos1,0, a[j], color[1]);
                draw_rectangle(xPos2,0, a[j+1], color[1]);
                wait_for_vsync();
                pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
                draw_rectangle(xPos1,0, a[j], color[1]);
                draw_rectangle(xPos2,0, a[j+1], color[1]);
            }

       }

   }

} 



/**********************************************************************************************************************
 * Takes in info of rect 1 (should be located on the left) and rect 2 (should be located on the right).
 * The animation is as follows:
 * Rect 2 will be pushed up
 * Rect 1 will be pushed to the right in place of rect 2
 * Rect 2 will be shifted to the left to align with old position of rect 1
 * Rect 2 will be lowered back down
 * *******************************************************************************************************************/
void animate_swap(int xPos1, int xPos2, int height1, int height2,  short int box_color1, short int box_color2){

    //Lifting Rect 2 up
    int startY = 0;
    int y;
    for (y = startY; y <= Y_LIMIT; y += ANIMATION_SPEED){

        //Erase previous rectangle
        if (y >= 2*ANIMATION_SPEED){
            draw_rectangle(xPos2, y - 2*ANIMATION_SPEED, height2, 0x0000); //Note that previous rectangle on the swapped buffer was two positions behind current position
        }
        else{
            draw_rectangle(xPos2, startY, height2, 0x0000); 
        }

        //Draw new rectangle
        draw_rectangle(xPos2, y, height2, HIGHLIGHT_COLOR);

        //swap buffers
        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer

        //Update location
        //done using the for loop
    }
    y = y-ANIMATION_SPEED; //store the last position of rect 2
    //Make sure image is aligned in the two buffers
    draw_rectangle(xPos2, y - ANIMATION_SPEED, height2, 0x0000);
    draw_rectangle(xPos2, Y_LIMIT, height2, HIGHLIGHT_COLOR);
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    draw_rectangle(xPos2, y, height2, 0x0000);
    y = Y_LIMIT;
    draw_rectangle(xPos2, y, height2, HIGHLIGHT_COLOR);

    //Move Rect 1 to the right
    int x;
    for (x = xPos1; x <= xPos2; x += ANIMATION_SPEED){

        //Erase previous rectangle
        if (x >= xPos1 + 2*ANIMATION_SPEED){
            draw_rectangle(x - 2*ANIMATION_SPEED, 0, height1, 0x0000); //Note that previous rectangle on the swapped buffer was two positions behind current position
        }
        else{
            draw_rectangle(xPos1, 0, height1, 0x0000);
        }

        //Draw new rectangle
        draw_rectangle(x, 0, height1, HIGHLIGHT_COLOR);

        //swap buffers
        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer

        //Update location
        //done using the for loop

    }
    x = x - ANIMATION_SPEED;
    draw_rectangle(x - ANIMATION_SPEED, 0, height1, 0x0000);
    draw_rectangle(xPos2, 0, height1, HIGHLIGHT_COLOR);
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    draw_rectangle(x, 0, height1, 0x0000);
    x = xPos2;
    draw_rectangle(x, 0, height1, HIGHLIGHT_COLOR);


    //Move Rect 2 to xPos 1
    for (x = xPos2; x >= xPos1; x -= ANIMATION_SPEED){

        //Erase previous rectangle
        if (x <= xPos2 - 2*ANIMATION_SPEED){
            draw_rectangle(x + 2*ANIMATION_SPEED, y, height2, 0x0000); //Note that previous rectangle on the swapped buffer was two positions behind current position        
        }
        else{
            draw_rectangle(xPos2, y, height2, 0x0000);
        }
        

        //Draw new rectangle
        draw_rectangle(x, y, height2, HIGHLIGHT_COLOR);

        //swap buffers
        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer

        //Update location
        //done using the for loop

    }
    x = x + ANIMATION_SPEED;
    draw_rectangle(x + ANIMATION_SPEED, y, height2, 0x0000);
    draw_rectangle(xPos1, y, height2, HIGHLIGHT_COLOR);
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    draw_rectangle(x, y, height2, 0x0000);
    x = xPos1;
    draw_rectangle(x, y, height2, HIGHLIGHT_COLOR);

    //Move Rect 2 back down
    for (y = Y_LIMIT; y >= 0; y -= ANIMATION_SPEED){

        //Erase previous rectangle
        if (y <= Y_LIMIT - 2*ANIMATION_SPEED){
            draw_rectangle(x, y + 2*ANIMATION_SPEED, height2, 0x0000); //Note that previous rectangle on the swapped buffer was two positions behind current position        
        }

        else{
            draw_rectangle(x, Y_LIMIT, height2, 0x0000);
        }
        

        //Draw new rectangle
        draw_rectangle(x, y, height2, HIGHLIGHT_COLOR);

        //swap buffers
        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer

        //Update location
        //done using the for loop

    }
    y = y + ANIMATION_SPEED;
    draw_rectangle(x, y + ANIMATION_SPEED, height2, 0x0000);
    draw_rectangle(x, 0, height2, HIGHLIGHT_COLOR);
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    draw_rectangle(x, y, height2, 0x0000);
    y = 0;
    draw_rectangle(x, y, height2, HIGHLIGHT_COLOR);


}

//Draw a rectangle centered at xPos and with height h.
void draw_rectangle(int xPos, int yPos, int height, short int box_color){
    
    for (int i = xPos - 13 ; i <= xPos + 13; i++){
        for(int j = 239 -yPos - height; j <= 239 -yPos; j++){
            plot_pixel(i, j, box_color);
        }
    }

}
