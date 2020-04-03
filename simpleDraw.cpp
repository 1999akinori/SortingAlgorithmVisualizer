#include <stdlib.h>
#include <time.h>

#define Y_LIMIT 120
#define ANIMATION_SPEED 17
#define MAX_ANIMATION_SPEED 20
#define HIGHLIGHT_COLOR 0xFFE0

#define N 10
#define HEIGHT 100
#define xBound 320
#define yBound 240
#define INITIAL 21
#define SPACE 31
#define WIDTH 27



//Gloabl Variables:
volatile int * timer_addr; 
volatile int pixel_buffer_start; 
volatile int * pixel_ctrl_ptr;
int numbers[N];
short color[10] = {0xFFFF, 0xF800, 0x07E0, 0x001F, 0x3478, 0xEAFF, 0x07FF, 0xF81F, 0xFFE0, 0x9909}; //List of colors to choose from

//Forward declarations:
void clear_screen();
void draw_rectangle(int xPos, int yPos, int height, short int box_color);
void animate_swap(int xPos1, int xPos2, int height1, int height2,  short int box_color1, short int box_color2);
void swap(int* x, int* y);
void bubble_sort(int a[], int size);
void wait_for_vsync();

void initialize();
void draw_array();


int main(void)
{

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

    bubble_sort(numbers, N);

    

}



void plot_pixel(int x, int y, short int line_color){
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}


//Draw a rectangle centered at xPos and with height h.
void draw_rectangle(int xPos, int yPos, int height, short int box_color){
    
    for (int i = xPos - 13 ; i <= xPos + 13; i++){
        for(int j = 239 -yPos - height; j <= 239 -yPos; j++){
            plot_pixel(i, j, box_color);
        }
    }

}

//Clears the screen by drawing black to all the pixels
void clear_screen(){
    for (int x = 0; x < 320; x++){
        for (int y = 0; y< 240; y++){
            plot_pixel(x, y, 0x0000);
        }
    }
}


//Call this function after writing 1 to the pixel_buffer_controller 
//in order to wait for buffer/backbuffer swap to occur
void wait_for_vsync(){
    register int status;

    *pixel_ctrl_ptr = 1; //Set the S bit to 1

    //Wait til the S bit turns into 0 meaning the last pixel reached
    status = *(pixel_ctrl_ptr + 3);
    while((status & 0x01) != 0){
        status = *(pixel_ctrl_ptr + 3);
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
                animate_swap(xPos1,xPos2,a[j], a[j+1] ,color[1], color[1]);

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





void initialize(){
    for(int i=0; i < N; i++){
        numbers[i] = rand()%HEIGHT + 1;
        //numbers[i] = 110 - i;
    }
}


void draw_array(){
    for(int i=0; i < N; i++){
        draw_rectangle(INITIAL + SPACE*i, 0, numbers[i], color[1]); 
    }
}