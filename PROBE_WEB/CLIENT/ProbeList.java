import java.util.*;
import java.awt.*;
import java.awt.event.*;

class ProbeList extends java.awt.List {
    private int        count             = 0;
    private NodeProbes probes;
    private String     error;
    private ProbesGUI  gui;
    private Probe      lastSelectedProbe;
    // private Probe      lastSelectedIndex = -1;

    public ProbeList(ProbesGUI gui, Color back_color, Font f) {
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
    public Probe getProbe(int index) { return probes == null ? null : probes.getProbe(index); }
    public Probe getSelectedProbe() { return lastSelectedProbe; }

    public int getSortedIndexOf(Probe p) { return probes == null ? -1 : probes.getSortedIndexOf(p); }
    public int getSortedIndexOf(String probeSeq) { return probes == null ? -1 : probes.getSortedIndexOf(probeSeq); }

    public void showOverlap(Probe p) throws Exception {
        if (Probe.setOverlapProbe(p)) {
            System.out.println("showOverlap refreshes List");

            // int old_index = getSelectedIndex();
            refreshList();
//             if (p == null) {
//                 selectProbe(old_index);
//             }
//             else {
//                 selectProbe(getSortedIndexOf(p));
//             }
        }
    }


    public void selectProbe(int index) throws Exception {
        if (index != -1) {
            // System.out.println("selectProbe index="+index);
            select(index);
            makeVisible(index);

            Probe p = getProbe(index);
            getGUI().getClient().matchProbes(p);
            if (p != null) lastSelectedProbe = p;
        }
        else {
            System.out.println("Details should be cleared."); // only if displaying probe details
        }
    }

    public void selectLastSelectedProbe() throws Exception {
        if (lastSelectedProbe != null) {
            int new_index = getSortedIndexOf(lastSelectedProbe);
            if (new_index == -1) {
                new_index = getSortedIndexOf(lastSelectedProbe.sequence());
                if (new_index == -1) {
                    new_index = 0; // select first
                }
            }
            // System.out.println("selectLastSelectedProbe (new_index="+new_index+")");
            selectProbe(new_index);
        }
        else {
            // System.out.println("selectLastSelectedProbe selects first element)");
            selectProbe(0);
        }
    }

    public void deselectCurrent() {
        int sel_idx = getSelectedIndex();
        if (sel_idx != -1) deselect(sel_idx);
    }

    private void refreshList() throws Exception {
        if (count != length()) {
            throw new Exception("count does not match listsize in 'refreshList'");
        }

        deselectCurrent();

        setVisible(false);
        for (int i = 0; i<count; i++) {
            add(getProbe(i).getDisplayString(), i);
            remove(i+1);
        }
        setVisible(true);
        selectLastSelectedProbe();
    }

    private void rebuildList() throws Exception {
        deselectCurrent();

        removeAll();
        if (count>0) {
            for (int i = 0; i<count; i++) {
                add(getProbe(i).getDisplayString());
            }
            selectLastSelectedProbe();
        }
        else {
            add("No probes found.");
        }
    }

    public void setContents(NodeProbes nodeProbes, String preferredProbe) throws Exception {
        probes = nodeProbes;
        probes.resortProbes();
        
        int preferredIndex = preferredProbe == null ? -1 : probes.getSortedIndexOf(preferredProbe);
        if (preferredIndex != -1) {
            lastSelectedProbe = getProbe(preferredIndex);
        }

        count = probes.size();
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
        if (mode == Probe.SORT_BY_OVERLAP) {
            if (Probe.getOverlapProbe() == null) {
                Toolkit.AbortWithError("No overlap active");
            }
        }
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
