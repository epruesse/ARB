//  ==================================================================== // 
//                                                                       // 
//    File      : TreeToolbar.java                                       // 
//    Purpose   : Toolbar for tree display                               // 
//    Time-stamp: <Sat Mar/13/2004 18:31 MET Coder@ReallySoft.de>        // 
//                                                                       // 
//                                                                       // 
//  Coded by Ralf Westram (coder@reallysoft.de) in March 2004            // 
//  Copyright Department of Microbiology (Technical University Munich)   // 
//                                                                       // 
//  Visit our web site at: http://www.arb-home.de/                       // 
//                                                                       // 
//  ==================================================================== // 

import java.awt.*;
import java.awt.event.*;

class TreeToolbar extends Panel {
    private ProbesGUI gui;
    private TextField searchText;

    public ProbesGUI gui() { return gui; }
    public TreeDisplay tree_display() { return gui.getTreeDisplay(); }

    public TreeToolbar(ProbesGUI g) {
        gui = g;
        setLayout(new FlowLayout(FlowLayout.LEFT, 1, 1));

        ActionListener buttonListener = new ActionListener(){
                public void actionPerformed(ActionEvent event) {
                    Button b   = (Button)event.getSource();
                    String cmd = b.getLabel();

                    if (cmd.equals("Back"))                 tree_display().previousRoot();
                    else if (cmd.equals("Root"))            tree_display().resetRoot();
                    else if (cmd.equals("Up"))              tree_display().goUp();
                    else if (cmd.equals("Show"))            tree_display().unfoldMarkedFoldRest();
                    else if (cmd.equals("Jump"))            tree_display().gotoRootOfMarked();
                    else if (cmd.equals("Expand"))          tree_display().smartUnfold();
                    else if (cmd.equals("Collapse"))        tree_display().smartFold();
                    else {
                        Toolkit.showError("Unknown button '"+cmd+"'");
                    }
                }
            };

        Button toAdd;
        toAdd = new Button("Back");         toAdd.addActionListener(buttonListener); add(toAdd);
        toAdd = new Button("Root");         toAdd.addActionListener(buttonListener); add(toAdd);
        toAdd = new Button("Up");           toAdd.addActionListener(buttonListener); add(toAdd);
        toAdd = new Button("Expand");       toAdd.addActionListener(buttonListener); add(toAdd);
        toAdd = new Button("Collapse");     toAdd.addActionListener(buttonListener); add(toAdd);

        add(new Label("Search:"));
        searchText = new TextField("", 10);
        searchText.addTextListener(new TextListener(){
                public void textValueChanged(TextEvent e) {
                    // System.out.println("textValueChanged getText="+searchText.getText());
                    tree_display().searchAndMark(searchText.getText());
                }
            });
        add(searchText);

        toAdd = new Button("Show");         toAdd.addActionListener(buttonListener); add(toAdd);
        toAdd = new Button("Jump");         toAdd.addActionListener(buttonListener); add(toAdd);

    }

//     public Dimension getMaximumSize() {
//         Dimension scdim = gui.getScrollPaneSize();
//         System.out.println("in TreeToolbar::getMaximumSize:");
//         System.out.println("scdim = "+scdim.width+"/"+scdim.height);
//         return new Dimension(scdim.width-10, 500);
//     }
     public Dimension getPreferredSize() {
         Dimension scdim  = gui.getPreferredScrollPaneSize();
         Dimension mypdim = super.getPreferredSize();

         // System.out.println("in TreeToolbar::getPreferredSize:");
         // System.out.println("scdim = "+scdim.width+"/"+scdim.height);
         // System.out.println("mypdim = "+mypdim.width+"/"+mypdim.height);

         if (mypdim.width > scdim.width) {
             if (scdim.width == 0) return mypdim;
             int lines = (int)(mypdim.width/scdim.width)+1;
             // System.out.println("TreeToolbar occupies "+lines+" lines.");

             Dimension myWantedDim = new Dimension(scdim.width-5, mypdim.height*lines+5);
             if (myWantedDim.height>scdim.height) {
                 myWantedDim.height = scdim.height; // do not get too tall
             }
             return myWantedDim;
         }

         return mypdim;
     }
}
