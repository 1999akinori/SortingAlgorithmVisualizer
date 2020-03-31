//Gloabl Variables:
volatile int pixel_buffer_start; 
volatile int * status_ptr;

//Forward declarations:
void delay(int milliseconds);
void clear_screen();
void draw_rectangle(int xPos, int height, short int box_color);
void animate_swap(int xPos1, int xPos2, int height1, int height2,  short int box_color1, short int box_color2);
void wait_for_vsync();


int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    status_ptr = pixel_ctrl_ptr + 3; // pointer to status register

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    *pixel_ctrl_ptr = 1; //Write 1 to controller to start synchronizing with the VGA
    wait_for_vsync();



    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer


    /*Clear both buffers before doing any drawing*/
    clear_screen(); //Clear buffer 1
    //swap buffers
    *pixel_ctrl_ptr = 1; //Write 1 to controller to start synchronizing with the VGA
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    clear_screen(); //Clear buffer 2
    //swap buffers
    *pixel_ctrl_ptr = 1; //Write 1 to controller to start synchronizing with the VGA
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer



    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer


    for (int x = 30; x < 200; x++){

        //Erase previous rectangle
        draw_rectangle(x - 2 , 200,  0x0000); //Note that previous rectangle on the swapped buffer was two positions behind current position
        
        //Draw new rectangle
        draw_rectangle(x, 200,   0x001F);

        //swap buffers
        *pixel_ctrl_ptr = 1; //Write 1 to controller to start synchronizing with the VGA
        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer

        //Update location
        //done using the for loop

    }

    // draw_rectangle(150, 200,  0x001F);
    // draw_rectangle(150, 200,  0x0000);
}



void draw_pixel(int x, int y, short int line_color){
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}


//Draw a rectangle centered at xPos and with height h.
void draw_rectangle(int xPos, int height, short int box_color){
    
    for (int i = xPos - 13 ; i <= xPos + 13; i++){
        for(int j = 239 - height; j <= 239; j++){
            draw_pixel(i, j, box_color);
        }
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






void animate_swap(int xPos1, int xPos2, int height1, int height2,  short int box_color1, short int box_color2){
    
}