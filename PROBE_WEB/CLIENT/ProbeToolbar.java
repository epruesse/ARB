//  ==================================================================== // 
//                                                                       // 
//    File      : ProbeToolbar.java                                      // 
//    Purpose   : Toolbar for application                                // 
//    Time-stamp: <Sun Mar/14/2004 22:29 MET Coder@ReallySoft.de>        // 
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

class ProbeToolbar extends Panel {
    private ProbesGUI gui;

    public ProbesGUI gui() { return gui; }
    public Client client() { return gui.getClient(); }
    public ProbeList probe_list() { return gui.getProbeList(); }
    private TreeDisplay tree_display() { return gui.getTreeDisplay(); }

    public ProbeToolbar(ProbesGUI g) {
        gui = g;

        setLayout(new BorderLayout());

        Panel row = new Panel(); row.setLayout(new FlowLayout(FlowLayout.LEFT));

        ActionListener buttonListener = new ActionListener(){
                public void actionPerformed(ActionEvent event) {
                    try {
                        Button b   = (Button)event.getSource();
                        String cmd = b.getLabel();

                        if (cmd.equals("Clear"))                client().clearMatches();
                        else if (cmd.equals("Save"))            client().saveProbes(true);
                        else if (cmd.equals("Overlap"))         gui().toggleOverlap();
                        else if (cmd.equals("Cache"))           tree_display().getLastMatchedNode().cacheAllHits();
                        
                        else if (cmd.equals("ABC"))             probe_list().setSort(Probe.SORT_BY_SEQUENCE);
                        else if (cmd.equals("Len"))             probe_list().setSort(Probe.SORT_BY_LENGTH);
                        else if (cmd.equals("Temp"))            probe_list().setSort(Probe.SORT_BY_TEMPERATURE);
                        else if (cmd.equals("GC"))              probe_list().setSort(Probe.SORT_BY_GC_CONTENT);
                        else if (cmd.equals("OL"))              probe_list().setSort(Probe.SORT_BY_OVERLAP);
                        else if (cmd.equals("Hits"))            probe_list().setSort(Probe.SORT_BY_NO_OF_HITS);
                        else {
                            Toolkit.showError("Unknown button '"+cmd+"'");
                        }
                    }
                    catch (ClientException ce) {
                        Toolkit.showError(ce.getMessage());
                    }
                    catch (Exception ex) {
                        Toolkit.showError("in itemStateChanged: "+ex.getMessage());
                        ex.printStackTrace();
                    }
                }
            };

        Button toAdd;
        toAdd = new Button("Clear");    toAdd.addActionListener(buttonListener); row.add(toAdd);
        toAdd = new Button("Save");     toAdd.addActionListener(buttonListener); row.add(toAdd);
        toAdd = new Button("Overlap");  toAdd.addActionListener(buttonListener); row.add(toAdd);
        toAdd = new Button("Cache");    toAdd.addActionListener(buttonListener); row.add(toAdd);

        add(row, BorderLayout.NORTH);
        row = new Panel(); row.setLayout(new FlowLayout(FlowLayout.LEFT));

        row.add(new Label("Sort"));

        toAdd = new Button("ABC");      toAdd.addActionListener(buttonListener); row.add(toAdd);
        toAdd = new Button("Len");      toAdd.addActionListener(buttonListener); row.add(toAdd);
        toAdd = new Button("Temp");     toAdd.addActionListener(buttonListener); row.add(toAdd);
        toAdd = new Button("GC");       toAdd.addActionListener(buttonListener); row.add(toAdd);
        toAdd = new Button("OL");       toAdd.addActionListener(buttonListener); row.add(toAdd);
        toAdd = new Button("Hits");     toAdd.addActionListener(buttonListener); row.add(toAdd);

        add(row, BorderLayout.SOUTH);
    }
}
