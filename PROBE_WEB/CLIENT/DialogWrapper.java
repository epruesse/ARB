// test code to experience file dialog capabilities
// generic wrapper class to provide a simple way to use modal dialogs
// parameters are the parent frame, a title and list of desired buttons
// get back the name string of the pressed button


import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.util.*;

public class DialogWrapper
implements ActionListener

{
    private Dialog dl;
    private String result;


public DialogWrapper(Frame parent, String title, Vector buttons){
    dl = new Dialog(parent, title, true);
    dl.setLayout(new BorderLayout());
    dl.setResizable(false);   
    Point parloc = parent.getLocation();
    // position inside parent window
    dl.setLocation(parloc.x + 30, parloc.y + 30);
    dl.add(new Label(title), BorderLayout.NORTH);
    Panel p = new Panel();
    p.setLayout(new FlowLayout(FlowLayout.CENTER));
    Enumeration en = buttons.elements();
    while (en.hasMoreElements()){
	//	System.out.println("Adding Button");
	Button aButton = new Button((String) en.nextElement());
	aButton.addActionListener(this);
	p.add(aButton, BorderLayout.EAST); 
    }
    dl.add(p, BorderLayout.SOUTH);
    dl.pack();
    dl.setVisible(true);

    }
     

    public String getResult(){
	return result;
    }



 public void actionPerformed(ActionEvent event)
    {
	result = event.getActionCommand();
	dl.setVisible(false);
	dl.dispose();
    }

    classe WClAdapter extends WindowAdapter{


    }

} // class DialogWrapper
