//  ==================================================================== // 
//                                                                       // 
//    File      : Probe.java                                             // 
//    Purpose   : Object holding one probe                               // 
//    Time-stamp: <Sat Mar/13/2004 19:41 MET Coder@ReallySoft.de>        // 
//                                                                       // 
//                                                                       // 
//  Coded by Ralf Westram (coder@reallysoft.de) in March 2004            // 
//  Copyright Department of Microbiology (Technical University Munich)   // 
//                                                                       // 
//  Visit our web site at: http://www.arb-home.de/                       // 
//                                                                       // 
//  ==================================================================== // 

import java.util.*;

public class Probe implements Comparable
{
    private String  probeString;
    private String  group_id;
    private boolean isexact = false;

    private int    temperature = -1;
    private double gc_content  = -1;
    private int    no_of_hits  = -1;

    public static GroupCache groupCache = null;

    private static Probe overlapProbe     = null;
    private static int   overlapProbeTime = 0;

    private String stripedProbeString     = null;
    private int    stripedProbeStringTime = -1;
    private int    overlapCount = 0;

    private static LinkedList compareOrder; // current compare order

    public static boolean setOverlapProbe(Probe p) {
        if (overlapProbe == null || !overlapProbe.equals(p)) { // new probe
//             if (p != null) System.out.println("setOverlapProbe: Set overlapProbe to '"+p.sequence()+"'");
//             else System.out.println("setOverlapProbe: Clear overlapProbe");

            overlapProbe = p;
            overlapProbeTime++;
//             System.out.println("setOverlapProbe: overlapProbeTime="+overlapProbeTime);
            return true;
        }
        return false;
    }
    public static Probe getOverlapProbe() {
        return overlapProbe;
    }

    public Probe(String probe_definition, TreeNode target) throws Exception {
        // System.out.println("probe_definition='"+probe_definition+"'");

        int komma1 = probe_definition.indexOf(',');
        if (komma1 != -1) {
            int komma2 = probe_definition.indexOf(',', komma1+1);
            if (komma2 == -1) {
                komma1 = -1;
            }
            else {
                probeString    = probe_definition.substring(0, komma1);
                group_id       = probe_definition.substring(komma2+1);
                String hitCode = probe_definition.substring(komma1+1, komma2);

                switch (hitCode.charAt(0)) {
                    case 'E':
                        isexact    = true;
                        no_of_hits = target.getNoOfLeaves();
                        break;

                    case 'C':
                        // hit_percentage = Integer.parseInt(hitCode.substring(1));
                        break;

                    default :
                        Toolkit.AbortWithError("Unknown hitcode '"+hitCode+"'");
                        break;

                }
            }
        }

        if (komma1 == -1) Toolkit.AbortWithError("comma expected in '"+probe_definition+"'");
    }


    public String sequence() { return probeString; }
    public String groupID() { return group_id; }
    public int length() { return probeString.length(); }

    public int temperature() {
        if (temperature == -1) {
            int length  = length();
            temperature = 0;
            for (int i = 0; i<length; ++i) {
                char c = probeString.charAt(i);
                temperature += ((c == 'A' || c == 'U') ? 2 : 4); // 2 + 4 rule
            }
        }
        return temperature;
    }
    public double gc_content() {
        if (gc_content == -1) {
            int length = length();
            gc_content = 0;
            for (int i = 0; i<length; ++i) {
                char c = probeString.charAt(i);
                if (c == 'G' || c == 'C') gc_content++;
            }
            gc_content = (gc_content*100.0)/length;
        }
        return gc_content;
    }

    public String members() throws Exception {
        return groupCache.getGroupMembers(groupID(), length());
    }

    public int no_of_hits() throws Exception {
        if (no_of_hits == -1) {
            String members = members();
            no_of_hits     = 1;
            int    komma   = members.indexOf(',');

            while (komma != -1) {
                komma = members.indexOf(',', komma+1);
                no_of_hits++;
            }
        }
        return no_of_hits;
    }

    private static String stripes = new String("====================");

    private static String stripeOut(String what, int from, int len) {
        if ((from+len) > what.length()) {
            System.out.println("Illegal parameters to stripeOut: what='"+what+"' what.length()="+what.length()+" from="+from+" len="+len);
        }

        return what.substring(0, from) +
            stripes.substring(0, len) +
            what.substring(from+len);
    }

    private String getStripedProbeString() {
        if (stripedProbeStringTime<overlapProbeTime) { // selected probe changed (or not initialized yet)
            if (overlapProbe == null || // no stripe out at all
                overlapProbe == this) // do not stripe out overlap-source
            {
                stripedProbeString = null;
                overlapCount       = overlapProbe == this ? 99 : 0;
            }
            else {
                String sel_probe = overlapProbe.probeString;
                int    found     = probeString.indexOf(sel_probe);

                if (found == -1) { // check for partial match
                    int     probe_len     = probeString.length();
                    int     sel_probe_len = sel_probe.length();

                    stripedProbeString = null;
                    overlapCount       = 0;

                    for (int i = probe_len; i>2; --i) {
                        if (probeString.regionMatches(false, 0, sel_probe, sel_probe_len-i, i)) { // end of selected probe matches at start of probe
                            stripedProbeString  = stripeOut(probeString, 0, i);
                            overlapCount       += i;
                            break;
                        }
                    }
                    for (int i = probe_len; i>2; --i) {
                        if (probeString.regionMatches(false, probe_len-i, sel_probe, 0, i)) { // start of selected probe matches at end of probe
                            stripedProbeString  = stripeOut(stripedProbeString == null ? probeString : stripedProbeString, probe_len-i, i);
                            overlapCount       += i;
                            break;
                        }
                    }
                }
                else {          // complete match
                    overlapCount       = overlapProbe.length();
                    stripedProbeString = stripeOut(probeString, found, overlapCount);
                }
            }
            stripedProbeStringTime = overlapProbeTime;

        }
        return stripedProbeString == null ? probeString : stripedProbeString;
    }

    public int getOverlap() {
        getStripedProbeString(); // calculates overlapCount
        return overlapCount;
    }

    public String getDisplayString() throws Exception {
        // System.out.println("getDisplayString");
        //         return group_id+" "+probeString+" "+getStripedProbeString();

        StringBuffer display = new StringBuffer();
        int          mode    = ((Integer)compareOrder.get(0)).intValue();

        if (mode<0) mode = -mode;

        switch (mode) {
            case SORT_BY_SEQUENCE: break;
            case SORT_BY_LENGTH:        display.append(length()); break;
            case SORT_BY_TEMPERATURE:   display.append(temperature()); display.append("°C"); break;
            case SORT_BY_GC_CONTENT:    display.append((int)(gc_content()+0.5)); display.append("%"); break;
            case SORT_BY_OVERLAP:       display.append(this == getOverlapProbe() ? length(): getOverlap()); break;
            case SORT_BY_NO_OF_HITS:    display.append(no_of_hits()); break;
        }

        if (display.length()>0)
            display.append(" ");
        display.append(getStripedProbeString());

        if (this == overlapProbe) {
            display.append(" (overlap seq.)");
        }
        return display.toString();
    }

    public static String getError() {
        return groupCache.getError();
    }

    public static void setCompareOrder(int cmp_mode) {
        Integer icmp_mode = new Integer(cmp_mode);

        if (compareOrder == null) {
            compareOrder = new LinkedList();
            compareOrder.addFirst(icmp_mode);
        }
        else {
            Integer minus_icmp_mode = new Integer(-cmp_mode); // negative means reverse
            if (icmp_mode.equals(compareOrder.get(0))) { // same as current first criteria ?
                // -> sort reverse
                compareOrder.remove(icmp_mode);
                compareOrder.addFirst(minus_icmp_mode);
            }
            else {
                compareOrder.remove(icmp_mode);
                compareOrder.remove(minus_icmp_mode);
                compareOrder.addFirst(icmp_mode);
            }
        }
    }

    private static int toCompareResult(double d) {
        return d<0 ? -1 : (d>0 ? 1 : 0);
    }

    // sort orders
    public static final int SORT_BY_SEQUENCE    = 1;
    public static final int SORT_BY_LENGTH      = 2;
    public static final int SORT_BY_TEMPERATURE = 3;
    public static final int SORT_BY_GC_CONTENT  = 4;
    public static final int SORT_BY_OVERLAP     = 5;
    public static final int SORT_BY_NO_OF_HITS  = 6;

    public int compareToByMode(Probe other, int mode) {
        switch (mode) {
            case SORT_BY_SEQUENCE:      return sequence().compareTo(other.sequence());
            case SORT_BY_LENGTH:        return length()-other.length();
            case SORT_BY_TEMPERATURE:   return temperature()-other.temperature();
            case SORT_BY_GC_CONTENT:    return toCompareResult(gc_content()-other.gc_content());
            case SORT_BY_OVERLAP:       return other.getOverlap()-getOverlap(); // sort reverse (max. overlap at top)
            case SORT_BY_NO_OF_HITS: {
                int cmp;
                try {
                    cmp = other.no_of_hits()-no_of_hits(); // sort reverse by no_of_hits (most hits at top)
                }
                catch (Exception ex) {
                    Toolkit.showError("in compareToByMode: "+ex.getMessage());
                    ex.printStackTrace();
                    cmp = 0;
                }
                return cmp;
            }
        }

        System.out.println("Unknown compare mode="+mode);
        return 0;
    }
    public int compareToByMode(Probe other, Integer imode) {
        int mode = imode.intValue();
        if (mode<0) return -compareToByMode(other, -mode);
        return compareToByMode(other, mode);
    }

    public int compareTo(Object obj) {
        Probe other = (Probe)obj;

        int cmp = compareToByMode(other, (Integer)compareOrder.get(0));
        if (cmp == 0) {
            int sort_orders = compareOrder.size();
            for (int s = 1; cmp == 0 && s<sort_orders; ++s) {
                cmp = compareToByMode(other, (Integer)compareOrder.get(s));
            }
        }
        return cmp;
    }
}
