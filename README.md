# GrinnellDoc

Below is a set of intructions to run GrinnellDoc.

 1) Open terminal and navigate to the root of the project.
 2) Use the command "make" to compile and link all the necessary files.
 3) Run "server filename.txt", where the file is the one being edited.
 4) Boot up another machine and navigate to the root of the project.
 5) Run "user [password] ServerIP" to connect to the server
 6) Once connected, the terminal should load the interface where the user can edit the file.
 7) To move the cursor on the screen, the user can use the left and right arrow keys.
 8) To add more users, repeat steps 4, 5, and 6.
 
 Example run:<br />
 Machine 1:<br />
 server test.txt <br />
 <br />
 Machine 2:<br />
 user IloveGrinnell 132.127.168.212<br />
  <br />
 Machine 3:<br />
 user IloveGrinnell 132.127.168.212<br />
 
 
 Notes:
 
  1) Please do not change the size of your terminal while running our program. You should maximize your terminal size.
  2) If nothing is printed or there are two copies of the text printed, don't panic. Move your cursor to the right and make any change. It will fix it.
  3) Please run the program on one account. After changing our buffers to stack memory buffers, it did not work out.
  4) Please create file first on the server side before run "server filename.txt
  
 Examples: our program behaves like Google Doc and normal text editors.
  
 Enjoy!
