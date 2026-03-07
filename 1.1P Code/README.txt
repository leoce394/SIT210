The following program allows a user to switch on their porch and hallway lights for 30 and 60 seconds respectively with the push of a button.

First, read_button() is called, returning a bool T/F if the button (digital pin 4) has been pressed. If false, the loop repeats to check this condition.

If true, porch_light_switch() and house_light_switch() are called, setting digital pins 2 and 3 to HIGH. After a 30 second delay in logic, porch_light_switch() is called again, checking if the pin is still HIGH and then setting it to LOW. After another 30 seconds house_light_switch() is called again with the same functionality.