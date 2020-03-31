#include <stdlib.h>
#include <stdbool.h>
#include<time.h>

//Gloabl Variables:
volatile int pixel_buffer_start; 
volatile int * status_ptr;

//Forward declarations:
void clear_screen();
void draw_line(int x0, int y0, int x1, int y1, short int line_color);
void draw_box(int x, int y, short int box_color);
void draw_pixel(int x, int y, short int line_color);
void wait_for_vsync();


int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    status_ptr = pixel_ctrl_ptr + 3; // pointer to status register
    /* Read location of the pixel buffer from the pixel buffer controller */

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    *pixel_ctrl_ptr = 1; //Write 1 to controller to start synchronizing with the VGA
    wait_for_vsync();

    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen();

    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer


     /* declare and initialize box arrays */
    int color_box[8], dx_box[8], dy_box[8], x_box[8], y_box[8];
    short color[10] = {0xFFFF, 0xF800, 0x07E0, 0x001F, 0x3478, 0xEAFF, 0x07FF, 0xF81F, 0xFFE0, 0x9909}; //List of colors to choose from

    // Use current time as seed for random generator 
    srand(time(0)); 

    for (int i = 0; i < 8; i++){
        x_box[i] = (rand()%311 + 5); //Random x position
        y_box[i] = (rand()%231 + 5); //Random y position

        dx_box[i] = (rand()%2)*2 - 1; //Random x increment
        dy_box[i] = (rand()%2)*2 - 1; //Random y increment

        color_box[i] = color[rand()%10];//Random color chosen from the color array
    }


    while(true){

        //Erase old screen
        clear_screen();

        //code for updating the locations of boxes
        for (int i = 0; i < 8; i++){

            //Check if any box has reached the horizontal edges
            if(x_box[i] == 4 || x_box[i] == 316){
                dx_box[i] = dx_box[i] * (-1);
            }

            //Check if any box has reached the vertical edges
            if(y_box[i] == 4 || y_box[i] == 236){
                dy_box[i] = dy_box[i] * (-1);
            }

            //Add the increments to the position cooridnates of the box
            x_box[i] += dx_box[i];
            y_box[i] += dy_box[i];
        }


        // code for drawing the boxes and lines
        for (int i = 0; i < 8; i++){
            draw_box(x_box[i], y_box[i], color_box[i]); //Draw the boxes
            draw_line(x_box[i], y_box[i], x_box[(i+1)%8], y_box[(i+1)%8], 0xF800); // Draw a blue line connecting them
        }


        /***********************************Synchronize Block*************************************/
        *pixel_ctrl_ptr = 1; //Write 1 to controller to start synchronizing with the VGA
        wait_for_vsync();

        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
        /***********************************Synchronize Block End**********************************/

        
    }

}


//Clears the screen by drawing black to all the pixels
void clear_screen(){
    for (int x = 0; x < 320; x++){
        for (int y = 0; y< 240; y++){
            draw_pixel(x, y, 0x0000);
        }
    }
}

void draw_line(int x0, int y0, int x1, int y1, short int line_color){

    //sway the x and y coordinates of each point if the line's slope is greater than 1
    bool is_steep = abs(y1 - y0) > abs(x1 - x0);
    if (is_steep){
        int temp = x0;
        x0 = y0;
        y0 = temp;
        temp = x1;
        x1 = y1;
        y1 = temp;
    }

    //Swap the points if the x's are not aligned
    if (x0 > x1){
        int temp = x0;
        x0 = x1;
        x1 = temp;
        temp = y0;
        y0 = y1;
        y1 = temp;
    }

    int deltax = x1 - x0;
    int deltay = abs(y1 - y0);
    int error = -(deltax / 2);
    int y = y0;
	int y_step;
    if (y0 < y1){
        y_step = 1;
    }
    else{
        y_step = -1;
    } 

    for (int x = x0; x <= x1; x++){
        if (is_steep)
            draw_pixel(y, x, line_color);
        else
            draw_pixel(x, y, line_color);

        error = error + deltay;
        if (error >= 0){
            y = y + y_step;
            error = error - deltax;
        }

    }

	return;
}


void draw_pixel(int x, int y, short int line_color){
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}


//Draw a 9 by 9 square to show a box
void draw_rectangle(int xPos, int height, short int box_color){
    
    for (int i = xPos - 13 ; i <= xPos + 13; i++){
        for(int j = 0; j <= height; j++){
            draw_pixel(i, j, box_color);
        }
    }

}

//Call this function after writing 1 to the pixel_buffer_controller 
//in order to wait for buffer/backbuffer swap to occur
void wait_for_vsync(){
    int status = *status_ptr;
    status = status & 1; // Bitwise and in order to isolate the s bit
    
    while(status){ //While loop waits for status to be set to 0, in order to synchronize with the VGA
        status = *status_ptr;
        status = status&1; // Bitwise and in order to isolate the s bit
    }
    return;
}