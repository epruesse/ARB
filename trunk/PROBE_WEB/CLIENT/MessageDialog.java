//  ==================================================================== // 
//                                                                       // 
//    File      : MessageDialog.java                                     // 
//    Purpose   :                                                        // 
//    Time-stamp: <Thu Mar/04/2004 10:35 MET Coder@ReallySoft.de>        // 
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
    //     String message;
    //     Button okButton;           // = new Button("OK");
    //     Label  messageLabel;
    boolean       done;
    Button        okButton;
    
    public MessageDialog(Frame frame, String title, String message)
    {
        super(frame, title);
//         this.message = message;
//         messageLabel = new Label(message);
//         okButton     = new Button("OK");

        done     = false;
        setLayout(new FlowLayout());
        //         add(messageLabel);
        add(new Label(message));
        //         add(okButton);
        okButton = new Button("OK");
        add(okButton);
        pack();
        setResizable(false);

        okButton.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent event) {
                    dispose();
                    done = true;
                }
            });

        okButton.addKeyListener(new KeyAdapter() {
                public void keyPressed(KeyEvent e) {
                    if (e.getKeyCode() == KeyEvent.VK_ENTER) {
                        dispose();
                        done = true;
                    }
                }
            });

        this.addWindowListener (new WindowAdapter() {
                public void windowClosing(WindowEvent event) {
                    dispose();
                    done = true;
                }
            });

        Dimension frameSize = frame.getSize();
        Point     frameLoc  = frame.getLocationOnScreen();

        int width  = this.getSize().width;
        int height = this.getSize().height;
        setLocation(frameLoc.x+(frameSize.width-width) / 2, frameLoc.y + (frameSize.height-height) / 2);
        toFront();
        show();

        done = false;
    }

    public boolean okClicked() {
        return done;
    }
}
