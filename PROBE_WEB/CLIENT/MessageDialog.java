//  ==================================================================== // 
//                                                                       // 
//    File      : MessageDialog.java                                     // 
//    Purpose   :                                                        // 
//    Time-stamp: <Thu Mar/11/2004 10:08 MET Coder@ReallySoft.de>        // 
//                                                                       // 
//                                                                       // 
//  Coded by Ralf Westram (coder@reallysoft.de) in March 2004            // 
//  Copyright Department of Microbiology (Technical University Munich)   // 
//                                                                       // 
//  Visit our web site at: http://www.arb-home.de/                       // 
//                                                                       // 
//                                                                       // 
//  ==================================================================== // 

import java.awt.*;
import java.awt.event.*;

public class MessageDialog extends Dialog
{
    boolean done;
    Frame   myFrame;

    public MessageDialog(Frame frame, String title, String message)
    {
        super(frame, title, false);

//         Window framewin = (Window)frame;
//         framewin.setFocusable(false);

        done    = false;
        myFrame = frame;

        setLayout(new BorderLayout());

        int lf = message.indexOf('\n');
        if (lf >= 0) {
            add(new Label(message.substring(0, lf)), BorderLayout.NORTH);
            add(new Label(message.substring(lf+1)), BorderLayout.CENTER);
        }
        else {
            add(new Label(message), BorderLayout.NORTH);
        }

        Button okButton = new Button("OK");
        {
            Panel buttonPanel = new Panel();
            buttonPanel.setLayout(new FlowLayout());
            buttonPanel.add(okButton);
            add(buttonPanel, BorderLayout.SOUTH);
        }

//         setResizable(false);

        okButton.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent event) {
//                     dispose();
//                     done = true;
                    setDone();
                }
            });

        okButton.addKeyListener(new KeyAdapter() {
                public void keyPressed(KeyEvent e) {
                    if (e.getKeyCode() == KeyEvent.VK_ENTER) {
                        setDone();
//                         dispose();
//                         done = true;
                    }
                }
            });

        this.addWindowListener (new WindowAdapter() {
                public void windowClosing(WindowEvent event) {
                    setDone();
//                     dispose();
//                     done = true;
                }
            });

        pack();

        Dimension frameSize = frame.getSize();
        Point     frameLoc  = frame.getLocationOnScreen();
        int       width     = this.getSize().width;
        int       height    = this.getSize().height;

        setLocation(frameLoc.x+(frameSize.width-width) / 2, frameLoc.y + (frameSize.height-height) / 2);

        toFront();
        show();

        myFrame.setEnabled(false);
    }

    private void setDone() {
        done = true;
        dispose();
        myFrame.setEnabled(true);
    }

    public boolean okClicked() {
        return done;
    }
}
