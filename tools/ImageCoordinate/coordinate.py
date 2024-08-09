from PIL import Image, ImageTk
import tkinter as tk
from tkinter import simpledialog
import argparse

# predifined image paths (for testing)
imgs = []

# Whether the previous rectangles will be cleared when creating a new one
KEEP_RECT = True

# The prompt message
PROMPT_TEXT = "Press 'q': quit, 'n': next image, 't': toggle text, 's': save last rect, 'c': clear all, 'i': input coords"

# resize the image to 1280x720. If the image is 16:9, scale it; if not, stretch it and return a false flag
def resize_image(image):
    # Get the width and height of the image
    w, h = image.size

    # If the image is 16:9, scale it to 1280x720
    if w / h == 16 / 9:
        image = image.resize((1280, 720))
        return image, True

    # If the image is not 16:9, stretch it to 1280x720
    image = image.resize((1280, 720), Image.BICUBIC)
    return image, False

class ImageRectSelector:
    def __init__(self, image_path):
        # Load the image using PIL
        self.image = Image.open(image_path)

        # resize the image to 1280x720
        self.image, self.is_16_9 = resize_image(self.image)

        # Create a Tkinter window
        self.root = tk.Tk()
        self.root.title("Image Rect Selector")

        # Create a Tkinter-compatible photo image, which can be used with the Canvas widget
        self.tk_image = ImageTk.PhotoImage(self.image)

        # Create a Canvas widget and display the photo image on it (leave some space below for the prompt message)
        self.canvas = tk.Canvas(
            self.root, width=self.image.width, height=self.image.height + 20)
        self.canvas.pack()
        self.canvas_image = self.canvas.create_image(
            0, 0, anchor=tk.NW, image=self.tk_image)

        # Print the prompt message below the image
        self.canvas.create_text(0, self.image.height + 20, text=PROMPT_TEXT, fill="black", anchor=tk.SW,
                                tags="prompt", font=("Helvetica", 14))

        # Bind the mouse event handlers to the Canvas widget
        self.canvas.bind("<Button-1>", self.on_mouse_down)
        self.canvas.bind("<B1-Motion>", self.on_mouse_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_mouse_up)

        # Bind the keyboard event handler to the root window
        self.root.bind("<Key>", self.on_key)

        # Set the flag so the program loops in run()
        self.is_quit = False

        # Set the flag if the whole program is quit (q pressed)
        self.is_quit_all = False

        # Set the flag if text widgets are hidden
        self.is_text_hidden = False

        # Initialize the rectangle coordinates
        self.rect_start_x, self.rect_start_y = None, None
        self.rect_end_x, self.rect_end_y = None, None

    # Switch the status of all text widgets between hidden and normal
    def toggle_text(self):
        # Get all text widgets
        text_widgets = self.canvas.find_withtag("text")

        # Toggle the flag
        self.is_text_hidden = not self.is_text_hidden

        # Hide or show all text widgets depending on the flag
        for text_widget in text_widgets:
            if self.is_text_hidden:
                self.canvas.itemconfig(text_widget, state=tk.HIDDEN)
            else:
                self.canvas.itemconfig(text_widget, state=tk.NORMAL)

    # Crop and save the last drawn rectangle
    def save_rect(self):
        # Get the coordinates of the rectangle
        x1, y1, x2, y2 = self.rect_start_x, self.rect_start_y, self.rect_end_x, self.rect_end_y

        # Get the height and width of the rectangle
        w, h = x2 - x1, y2 - y1

        # Crop the image using the coordinates
        rect = self.image.crop((x1, y1, x2, y2))

        # Save the cropped image
        rect.save(f"Cropped_rect_{x1}_{y1}_{w}_{h}.png")

    def draw_rectangle(self, tags="rect"):
        # Erase previous temporary rectangles
        self.canvas.delete("tmp")

        # Erase the previous rectangles if KEEP_RECT is set to False
        if not KEEP_RECT:
            self.canvas.delete("rect")

        # Draw the rectangle with the given tags
        self.canvas.create_rectangle(self.rect_start_x, self.rect_start_y, self.rect_end_x, self.rect_end_y,
                                     outline='red', tags=tags)

    def on_mouse_down(self, event):
        self.rect_start_x, self.rect_start_y = event.x, event.y

    def on_mouse_drag(self, event):
        # Draw the new temporary rectangle on the canvas
        self.rect_end_x, self.rect_end_y = event.x, event.y
        self.draw_rectangle("tmp")

    def on_mouse_up(self, event):
        # Draw the new rectangle on the canvas
        self.rect_end_x, self.rect_end_y = event.x, event.y
        self.draw_rectangle()

        # Print the rectangle coordinates
        coords = f"{self.rect_start_x}, {self.rect_start_y}, {self.rect_end_x - self.rect_start_x}, {self.rect_end_y - self.rect_start_y}"
        print(coords)

        # Create a background rectangle for the text widget
        self.canvas.create_rectangle(self.rect_start_x, self.rect_start_y, self.rect_start_x + 130, self.rect_start_y + 20,
                                     fill="white", outline="white", tags="text", state=tk.HIDDEN if self.is_text_hidden else tk.NORMAL)

        # Create a text widget with white background to display the rectangle coordinates on the canvas
        self.canvas.create_text(self.rect_start_x, self.rect_start_y, text=coords,
                                fill="black", anchor=tk.NW, tags="text", font=("Helvetica", 10), state=tk.HIDDEN if self.is_text_hidden else tk.NORMAL)

    def on_key(self, event):
        # Handle the 'q' key to quit the program
        if event.char == "q":
            self.is_quit = True
            self.is_quit_all = True

            # Delete the tk window
            self.root.destroy()
            # self.root.quit()

        # Handle the 'n' key to load the next image
        elif event.char == "n":
            self.is_quit = True

            # Delete the tk window
            self.root.destroy()
            # self.root.quit()

        # Handle the 'c' key to clear all rectangles
        elif event.char == "c":
            self.canvas.delete("rect")
            self.canvas.delete("tmp")
            self.canvas.delete("text")

        # Handle the 'i' key to take an input and draw the corresponding rectangle
        elif event.char == "i":
            self.prompt_rect_coords()
            self.draw_rectangle()

        # Handle the 't' key to toggle the text widget
        elif event.char == "t":
            self.toggle_text()

        # Handle the 's' key to save the last drawn rectangle
        elif event.char == "s":
            self.save_rect()

    def prompt_rect_coords(self):
        # Prompt the user to enter the rectangle coordinates
        coords_str = simpledialog.askstring(
            "Input", "Enter rectangle coordinates (x1 y1 x2 y2):")
        x, y, w, h = map(int, coords_str.replace(',', ' ').split())

        # Set the rectangle coordinates
        self.rect_start_x, self.rect_start_y = x, y
        self.rect_end_x, self.rect_end_y = x + w, y + h

    def run(self):
        # If the image is not 16:9, show a message box and continue
        if not self.is_16_9:
            tk.messagebox.showinfo("Warning", "The image is not 16:9! Proceeding to next image...")
            self.root.destroy()
            return

        # Start the Tkinter event loop and periodically check for user input
        while not self.is_quit:
            self.root.update_idletasks()
            self.root.update()
            self.root.after(10)  # wait 10 ms between event loop iterations


if __name__ == "__main__":
    import os

    # get the path of the current file
    current_file_path = os.path.abspath(__file__)

    # get the directory containing the current file
    current_dir_path = os.path.dirname(current_file_path)

    # set the current working directory to the directory containing the current file so that we can save the cropped images in the same directory
    os.chdir(current_dir_path)

    # Define command-line arguments
    parser = argparse.ArgumentParser(
        description='Select a rectangle on an image.')
    parser.add_argument('image_paths', type=str,
                        nargs='*',
                        help='Path to the input image file.')

    # Parse command-line arguments
    args = parser.parse_args()

    # Handle image path in different situations
    if args.image_paths:
        image_paths = args.image_paths
    elif imgs:
        image_paths = imgs
    else:
        imgs = simpledialog.askstring(
            "Input", "Enter image path(s):")
        image_paths = imgs.replace(',', ' ').split()

    # Print the prompt
    print(PROMPT_TEXT)

    # Create an ImageRectSelector instance for each image path
    for image_path in image_paths:

        # Create an ImageRectSelector instance with the image path
        selector = ImageRectSelector(image_path)

        # Run the selector
        selector.run()

        # break if the user presses the 'q' key
        if selector.is_quit_all:
            # delete the class instance
            del selector

            break

        # delete the class instance
        del selector
