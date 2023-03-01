from PIL import Image, ImageTk
import tkinter as tk
import argparse

img = ""

class ImageRectSelector:
    def __init__(self, image_path):
        # Load the image using PIL
        self.image = Image.open(image_path)

        # Create a Tkinter window
        self.root = tk.Tk()

        # Create a Tkinter-compatible photo image, which can be used with the Canvas widget
        self.tk_image = ImageTk.PhotoImage(self.image)

        # Create a Canvas widget and display the photo image on it
        self.canvas = tk.Canvas(
            self.root, width=self.image.width, height=self.image.height)
        self.canvas.pack()
        self.canvas_image = self.canvas.create_image(
            0, 0, anchor=tk.NW, image=self.tk_image)

        # Bind the mouse event handlers to the Canvas widget
        self.canvas.bind("<Button-1>", self.on_mouse_down)
        self.canvas.bind("<B1-Motion>", self.on_mouse_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_mouse_up)

        # Bind the keyboard event handler to the root window
        self.root.bind("<Key>", self.on_key)

        self.is_quit = False

        # Initialize the rectangle coordinates
        self.rect_start_x, self.rect_start_y = None, None
        self.rect_end_x, self.rect_end_y = None, None

    def draw_rectangle(self):
        self.canvas.create_rectangle(self.rect_start_x, self.rect_start_y, self.rect_end_x, self.rect_end_y,
                                     outline='red', tags="rect")

    def on_mouse_down(self, event):
        self.rect_start_x, self.rect_start_y = event.x, event.y

    def on_mouse_drag(self, event):
        # Erase the previous rectangle
        self.canvas.delete("rect")

        # Draw the new rectangle on the canvas
        self.rect_end_x, self.rect_end_y = event.x, event.y
        self.draw_rectangle()

    def on_mouse_up(self, event):
        # Erase the final rectangle
        self.canvas.delete("rect")

        # Draw the new rectangle on the canvas
        self.rect_end_x, self.rect_end_y = event.x, event.y
        self.draw_rectangle()

        # Update the PhotoImage with the drawn rectangle
        # self.image.paste(self.canvas_image, mask=self.canvas_image)

        # Print the rectangle coordinates
        print(f"({self.rect_start_x}, {self.rect_start_y}), ({self.rect_end_x - self.rect_start_x}, {self.rect_end_y - self.rect_start_y})")

    def on_key(self, event):
        # Handle the 'q' key to quit the program
        if event.char == "q":
            self.is_quit = True
            self.root.quit()
        elif event.char == "c":
            self.canvas.delete("rect")
        elif event.char == "i":
            self.prompt_rect_coords()
            self.draw_rectangle()

    def prompt_rect_coords(self):
        # Prompt the user to enter the rectangle coordinates
        x, y, w, h = input(
            "Enter the rectangle coordinates (x y w h): ").split()
        x, y, w, h = int(x), int(y), int(w), int(h)

        # Set the rectangle coordinates
        self.rect_start_x, self.rect_start_y = x, y
        self.rect_end_x, self.rect_end_y = x + w, y + h

    def run(self):
        # Start the Tkinter event loop and periodically check for user input
        while not self.is_quit:
            self.root.update_idletasks()
            self.root.update()
            self.root.after(10)  # wait 10 ms between event loop iterations

    # def run(self):
    #     # Start the Tkinter event loop
    #     self.root.mainloop()


if __name__ == "__main__":
    import os

    # get the path of the current file
    current_file_path = os.path.abspath(__file__)

    # get the directory containing the current file
    current_dir_path = os.path.dirname(current_file_path)

    # set the current working directory to the directory containing the current file
    os.chdir(current_dir_path)

    # Define command-line arguments
    parser = argparse.ArgumentParser(
        description='Select a rectangle on an image.')

    parser.add_argument('image_path', type=str,
                        nargs='?',
                        help='Path to the input image file.')

    # Parse command-line arguments
    args = parser.parse_args()

    if args.image_path:
        image_path = args.image_path
    elif img:
        image_path = img
    else:
        image_path = input("Enter path to image file: ")

    # Create an ImageRectSelector instance with the image path
    selector = ImageRectSelector(image_path)

    # Run the selector
    selector.run()
