// test code to experience file dialog capabilities
// generic wrapper class to provide a simple way to use modal dialogs
// parameters are the parent frame, a title and list of desired buttons
// get back the name string of the pressed button


import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.util.*;

public class DialogWrapper implements ActionListener
{
    private Dialog dl;
    private String result;


    public DialogWrapper(Frame parent, String title, String text, Vector buttons){
        dl = new Dialog(parent, title, true);
        dl.setLayout(new BorderLayout());
        // dl.setResizable(true);
        
        Point parloc = parent.getLocation();
        // position inside parent window
        dl.setLocation(parloc.x + 100, parloc.y + 100);

        Component c = null;
        {
            int lf = text.indexOf('\n');
            if (lf == -1) { // oneliner
                Label t = new Label(text);
                c       = t;
            }
            else {
                Panel p = new Panel();

                p.setLayout(new BorderLayout());
                p.add(new Label(text.substring(0, lf)), BorderLayout.NORTH);
                p.add(new Label(text.substring(lf+1)), BorderLayout.SOUTH);

                c = p;
            }
        }
        dl.add(c, BorderLayout.NORTH);

        Panel p = new Panel();
        p.setLayout(new FlowLayout());
        {
            Enumeration en = buttons.elements();
            while (en.hasMoreElements()){
                //  System.out.println("Adding Button");
                Button aButton = new Button((String) en.nextElement());
                aButton.addActionListener(this);
                p.add(aButton);
            }
        }
        dl.add(p, BorderLayout.SOUTH);

        dl.pack();
        {
            Dimension cd = c.getSize();
            Dimension pd = p.getSize();

            // System.out.println("cd="+cd.width+"/"+cd.height);
            // System.out.println("pd="+pd.width+"/"+pd.height);

            dl.setSize((cd.width > pd.width ? pd.width : cd.width)+50,
                       cd.height+pd.height+25);

            // Dimension dld = dl.getSize();
            // System.out.println("dld="+dld.width+"/"+dld.height);
        }

        dl.pack();

        // Dimension dld = dl.getSize();
        // System.out.println("dld="+dld.width+"/"+dld.height);

        dl.setResizable(false);
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

//     class WClAdapter extends WindowAdapter{


//     }

} // class DialogWrapper
