// driver for DialogWrapper class
// (c) Lothar Richter March 2004
// part of the ARB project

import java.awt.*;
import java.util.*;

public class DialogDriver
{



    public static void main(String[] argc){
 	Frame tf = new Frame("test frame");
	tf.setLocation(200,200);
	tf.setSize(400, 600);
	tf.setVisible(true);
	Vector buttons = new Vector();
	buttons.addElement(new String("Save"));
	buttons.addElement(new String("Discard"));
	buttons.addElement(new String("Cancel"));
 	DialogWrapper  dw = new DialogWrapper( tf, "dialog title", buttons);




	System.out.println(dw.getResult());

	System.exit(0);

     }
}
