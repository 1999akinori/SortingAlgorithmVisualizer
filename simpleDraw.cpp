#include <stdlib.h>
#include <time.h>

#define Y_LIMIT 120
#define ANIMATION_SPEED 5
#define MAX_ANIMATION_SPEED 20
#define HIGHLIGHT_COLOR 0xFFE0
#define HIGHLIGHT_PIVOT 0xA81E
#define HIGHLIGHT_DONE 0x1F60
#define HIGHLIGHT_SMALLEST_QUICKSORT 0x077D

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
short color[10] = {0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800}; //List of colors to choose from

//Forward declarations:
void clear_screen();
void draw_rectangle(int xPos, int yPos, int height, short int box_color);
void animate_swap(int xPos1, int xPos2, int height1, int height2,  short int box_color1, short int box_color2);
void animate_double_swap(int xPos1, int xPos2, int height1, int height2,  short int box_color1, short int box_color2);
void swap(int* x, int* y);
void bubble_sort(int a[], int size);
void quick_sort(int a[], int low, int high);
void wait_for_vsync();

void initialize();
void draw_array();


int main(void){

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

    //Initiaize array to all reds
    for (int i = 0; i < N; i ++){
        color[i] = 0xF800;
    }

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

            if(j == size - i - 2){
                //Highlight the rectangles as done
                color[j+1] = HIGHLIGHT_DONE;
                draw_rectangle(xPos2,0, a[j+1], color[j+1]);
                wait_for_vsync();
                pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
                draw_rectangle(xPos2,0, a[j+1], color[j+1]);
            }

       }

   }

    //Highlight the rectangles as done
    color[0] = HIGHLIGHT_DONE;
    draw_rectangle(INITIAL,0, a[0], color[0]);
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    draw_rectangle(INITIAL,0, a[0], color[0]);

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
        draw_rectangle(INITIAL + SPACE*i, 0, numbers[i], color[i]); 
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
        draw_rectangle(xPos1, y, height1, box_color1);
        draw_rectangle(xPos2, y, height2, box_color2);

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
    draw_rectangle(xPos1, Y_LIMIT, height1, box_color1);
    draw_rectangle(xPos2, Y_LIMIT, height2, box_color2);
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    draw_rectangle(xPos1, y, height1, 0x0000);
    draw_rectangle(xPos2, y, height2, 0x0000);
    y = Y_LIMIT;
    draw_rectangle(xPos1, y, height1, box_color1);
    draw_rectangle(xPos2, y, height2, box_color2);


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
        draw_rectangle(x2, y, height2, box_color2);
        draw_rectangle(x1, y, height1, box_color1);

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
    draw_rectangle(xPos1, y, height2, box_color2);
    draw_rectangle(xPos2, y, height1, box_color1);
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    draw_rectangle(x2, y, height2, 0x0000);
    draw_rectangle(x1, y, height1, 0x0000);
    x2 = xPos1;
    x1 = xPos2;
    draw_rectangle(x2, y, height2, box_color2);
    draw_rectangle(x1, y, height1, box_color1);

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
        draw_rectangle(x2, y, height2, box_color2);
        draw_rectangle(x1, y, height1, box_color1);

        //swap buffers
        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer

        //Update location
        //done using the for loop

    }
    y = y + ANIMATION_SPEED;
    draw_rectangle(x2, y + ANIMATION_SPEED, height2, 0x0000);
    draw_rectangle(x1, y + ANIMATION_SPEED, height1, 0x0000);
    draw_rectangle(x2, 0, height2, box_color1);
    draw_rectangle(x1, 0, height1, box_color2);
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    draw_rectangle(x2, y, height2, 0x0000);
    draw_rectangle(x1, y, height1, 0x0000);
    y = 0;
    draw_rectangle(x2, y, height2, box_color1);
    draw_rectangle(x1, y, height1, box_color2);


}


/* This function takes last element as pivot, places 
   the pivot element at its correct position in sorted 
    array, and places all smaller (smaller than pivot) 
   to left of pivot and all greater elements to right 
   of pivot */
int partition (int a[], int low, int high) { 
    int pivot = a[high];    // pivot 
    int i = (low - 1);  // Index of smaller element 
  
    //Highlight the pivot
    int xPos1Pi = INITIAL + SPACE*high;
    draw_rectangle(xPos1Pi,0, pivot, HIGHLIGHT_PIVOT);
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    draw_rectangle(xPos1Pi,0, pivot, HIGHLIGHT_PIVOT);

    for (int j = low; j <= high- 1; j++) 
    { 
        //Highlight the rectangle
        int selectedRectPos = INITIAL + SPACE*j;
        draw_rectangle(selectedRectPos,0, a[j], HIGHLIGHT_COLOR);
        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
        draw_rectangle(selectedRectPos,0, a[j], HIGHLIGHT_COLOR);
        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer

        
        // If current element is smaller than the pivot 
        if (a[j] < pivot) 
        { 
            i++;    // increment index of smaller element 


            //Wait
            *(timer_addr) = 50000000 * (MAX_ANIMATION_SPEED / ANIMATION_SPEED); 
            *(timer_addr + 2) = 0b1; //Enable timer
            while (*(timer_addr + 3) != 1){
                //wait
            }
            *(timer_addr + 3) = 1; //Reset F bit to 0;


            //Don't swap if at the same location
            if (i == j){
                //Highlight the smallest element in blue
                int smallestXpos = INITIAL + SPACE*i;
                draw_rectangle(smallestXpos,0, a[i], HIGHLIGHT_SMALLEST_QUICKSORT);
                wait_for_vsync();
                pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
                draw_rectangle(smallestXpos,0, a[i], HIGHLIGHT_SMALLEST_QUICKSORT);
                wait_for_vsync();
                pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
                continue;
            }

            //Animate the swap of the rectangles
            int xPos1 = INITIAL + SPACE*i;
            int xPos2 = INITIAL + SPACE*j;
            animate_double_swap(xPos1,xPos2,a[i], a[j] , color[i], color[j]);

            swap(&a[i], &a[j]); 

            //Highlight the smallest element in blue
            int smallestXpos = INITIAL + SPACE*i;
            draw_rectangle(smallestXpos,0, a[i], HIGHLIGHT_SMALLEST_QUICKSORT);
            wait_for_vsync();
            pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
            draw_rectangle(smallestXpos,0, a[i], HIGHLIGHT_SMALLEST_QUICKSORT);
            wait_for_vsync();
            pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer

        } 
        else{
            //Wait
            *(timer_addr) = 50000000 * (MAX_ANIMATION_SPEED / ANIMATION_SPEED); 
            *(timer_addr + 2) = 0b1; //Enable timer
            while (*(timer_addr + 3) != 1){
                //wait
            }
            *(timer_addr + 3) = 1; //Reset F bit to 0;

            //Unhighlight 
            int selectedRectPos = INITIAL + SPACE*j;
            draw_rectangle(selectedRectPos,0, a[j], color[j]);
            wait_for_vsync();
            pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
            draw_rectangle(selectedRectPos,0, a[j], color[j]);
            wait_for_vsync();
            pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
        }



        
    } 

    //Wait
    *(timer_addr) = 50000000 * (MAX_ANIMATION_SPEED / ANIMATION_SPEED); 
    *(timer_addr + 2) = 0b1; //Enable timer
    while (*(timer_addr + 3) != 1){
        //wait
    }
    *(timer_addr + 3) = 1; //Reset F bit to 0;
    //Animate the swap of the pivot 
    if(i+1 != high){
        int xPos1 = INITIAL + SPACE*(i+1);
        int xPos2 = INITIAL + SPACE*(high);
        animate_double_swap(xPos1,xPos2,a[i + 1], a[high] ,color[i+1], color[high]);
        swap(&a[i + 1], &a[high]); 
    }


    //Indicate that the pivot has been placed in the right position
    color[i+1] = HIGHLIGHT_DONE;
    draw_array();
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    draw_array();
    return (i + 1); 
} 
  

void quick_sort(int a[], int low, int high) 
{ 
    if (low <= high){ 
        /* pi is partitioning index, arr[p] is now 
           at right place */
        int pi = partition(a, low, high); 
    
        // Separately sort elements before 
        // partition and after partition 
        quick_sort(a, low, pi - 1); 
        quick_sort(a, pi + 1, high); 
    } 

}