



//Gloabl Variables:
volatile int pixel_buffer_start; 

//Forward declarations:
void clear_screen();
void draw_rectangle(int x, int y, short int box_color);


int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    /* Read location of the pixel buffer from the pixel buffer controller */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen();
    draw_rectangle(150, 200,  0x001F);
}








void draw_pixel(int x, int y, short int line_color){
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}


//Draw a 9 by 9 square to show a box
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