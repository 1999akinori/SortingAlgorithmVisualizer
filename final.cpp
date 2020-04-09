// Libraries
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#define PS2_BASE 0xFF200100

// Sorting Related
#define Y_LIMIT 120
#define ANIMATION_SPEED 17
#define MAX_ANIMATION_SPEED 20
#define HIGHLIGHT_COLOR 0xFFE0
#define HIGHLIGHT_PIVOT 0xA81E
#define HIGHLIGHT_DONE 0x1F60
#define HIGHLIGHT_SMALLEST_QUICKSORT 0x077D
	
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
short color[10] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}; //List of colors to choose from
bool sortType;

//Forward Declaration
void HEX_PS2(char, char, char);
void initialize();
void clear_screen();
void wait_for_vsync();
void plot_pixel(int x, int y, short int line_color);
void draw_rectangle(int xPos, int yPos, int height, short int box_color);
void draw_array();
void draw_array_initial();
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
    
    //Bubble sort by default
    sortType = true;

    while(true){
        initialize(); //Initialize array
        /*Clear both buffers before doing any drawing*/
		for (int i = 0; i < N; i ++){
			color[i] = 0xF800;
		}
        clear_screen();
        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
        clear_screen();


        int PS2_data, RVALID, RAVAIL;
        char byte1 = 0, byte2 = 0, byte3 = 0; // Stores the data from the key
        *(PS2_ptr) = 0xFF; // reset

        current = 0;
        
        //When enter is placed the process it done
        bool enter = false;

        RAVAIL = *(PS2_ptr) & 0xFFFF0000; // isolate the RAVAIL values

        while(RAVAIL != 0){ // Wait until the FIFO is cleared
            PS2_data = *(PS2_ptr);
            RVALID = PS2_data & 0x8000;
            RAVAIL = *(PS2_ptr) & 0xFFFF0000;
        }

        while (!enter) {
            PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port
            RVALID = PS2_data & 0x8000; // extract the RVALID field
           
            if (RVALID) {
                /* shift the next data byte into the display */
                // byte1 byte2 byte3
                byte1 = byte2;
                byte2 = byte3;
                byte3 = PS2_data & 0xFF; //New data
            }
            if(byte1 == 0x5A | byte2 == 0x5A | byte3 == 0x5A) enter = true;
            if(byte1 == 0x32 | byte2 == 0x32 | byte3 == 0x32) sortType = true;
            if(byte1 == 0x15 | byte2 == 0x15 | byte3 == 0x15) sortType = false;
            if(byte3 == 0xF0){ // The FIFO is empty and all input have been recieved
                int instruction = 0;
                if(byte1 == 0x75) instruction = UP;
                if(byte1 == 0x6B) instruction = LEFT;
                if(byte1 == 0x74) instruction = RIGHT;
                if(byte1 == 0x72) instruction = DOWN;
                draw_rectangle(INITIAL+current*SPACE, 0, numbers[current], 0x0000);
                wait_for_vsync(); // swap front and back buffers on VGA vertical sync
                pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
                draw_rectangle(INITIAL+current*SPACE, 0, numbers[current], 0x0000);
                modify_array(instruction);
                byte1 = 0; // terminate instructions
                byte2 = 0;
                byte3 = 0;
                draw_array_initial();
                wait_for_vsync(); // swap front and back buffers on VGA vertical sync
                pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
				draw_array_initial();

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

        if(sortType){
            bubble_sort(numbers, N);
        }else{
            quick_sort(numbers, 0, N-1);
        }
		
		current = 0;
		//Initiaize array to all reds
		//for (int i = 0; i < N; i ++){
		//	color[i] = 0xF800;
		//}

    }
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
            draw_rectangle(INITIAL + SPACE*i,0, numbers[i], color[i]); 
        }else{
            draw_rectangle(INITIAL + SPACE*i,0, numbers[i], color[i]); 
        }
    }
}

void draw_array_initial(){
    for(int i=0; i < N; i++){
        if(i == current){
            draw_rectangle(INITIAL + SPACE*i,0, numbers[i], 0xFFE0); 
        }else{
            draw_rectangle(INITIAL + SPACE*i,0, numbers[i], 0xF800); 
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
    if (low <= high) 
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

//Draw a rectangle centered at xPos and with height h.
void draw_rectangle(int xPos, int yPos, int height, short int box_color){
    for (int i = xPos - 13 ; i <= xPos + 13; i++){
        for(int j = 239 -yPos - height; j <= 239 -yPos; j++){
            plot_pixel(i, j, box_color);
        }
    }

}

	
	
	