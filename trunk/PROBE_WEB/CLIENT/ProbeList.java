import java.util.*;
import java.awt.*;
import java.awt.event.*;

class ProbeList extends java.awt.List {
    private int             count = 0;
    private NodeProbes      probes;
    private String          error;
    private ProbesGUI gui;

    public ProbeList(ProbesGUI gui, Color back_color) {
        this.gui = gui;
        setBackground(back_color);
        setVisible(true);
        addItemListener(new ItemListener() {
                public void itemStateChanged(ItemEvent e) {
                    if (e.getStateChange() == ItemEvent.SELECTED) {
                        try {
                            selectProbe(getSelectedIndex());
                        }
                        catch (ClientException ce) {
                            Toolkit.showError(ce.getMessage());
                        }
                        catch (Exception ex) {
                            Toolkit.showError("in itemStateChanged: "+ex.getMessage());
                            ex.printStackTrace();
                        }
                    }

                }
            });
    }

    public int length() { return count; }

    public boolean hasError() { return error != null; }
    public String getError() { return error; }

    public ProbesGUI getGUI() { return gui; }

//     public int getSelectedIndex() { return getSelectedIndex(); }
    public Probe getProbe(int index) {
        return probes == null
            ? null
            : probes.getProbe(index);
    }

    public void showOverlap(Probe p) throws Exception {
        if (Probe.setOverlapProbe(p)) {
            System.out.println("showOverlap refreshes List");
            int index = getSelectedIndex();
            
            refreshList();
            select(index);
        }
    }


    public void selectProbe(int index) throws Exception {
        Probe p = getProbe(index);
        select(index);
        getGUI().getClient().matchProbes(p);
    }

    private void refreshList() throws Exception {
        if (count != length()) {
            throw new Exception("count does not match listsize in 'refreshList'");
        }

        setVisible(false);
        for (int i = 0; i<count; i++) {
            add(getProbe(i).getDisplayString(), i);
            remove(i+1);
        }
        setVisible(true);
    }

    private void rebuildList() throws Exception {
        removeAll();
        if (count>0) {
            for (int i = 0; i<count; i++) {
                add(getProbe(i).getDisplayString());
            }
        }
        else {
            add("No probes found.");
        }
    }

    public void setContents(NodeProbes nodeProbes) throws Exception {
        probes = nodeProbes;
        count  = probes.size();
        rebuildList();
    }

    public void clearContents() {
        probes = null;
        count  = 0;
        removeAll();
    }

    public void resort() throws Exception {
        if (probes != null) {
            probes.resortProbes();
            refreshList();
            // @@@ FIXME: select previously selected probe!
        }
    }
    public void setSort(int mode) throws Exception {
        Probe.setCompareOrder(mode);
        resort();
    }
    public String getProbesAsString() throws Exception {
        StringBuffer pstrb = new StringBuffer();
        for (int i = 0; i<count; i++) {
            Probe probe = getProbe(i);
            if (probe != null) {
                pstrb.append(probe.getDisplayString() + "\n");
            }
            else {
                pstrb.append("--- Error: No probe! ---" + "\n");
            }
        }
        return pstrb.toString();
    }

}
