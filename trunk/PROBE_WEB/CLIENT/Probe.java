//  ==================================================================== // 
//                                                                       // 
//    File      : Probe.java                                             // 
//    Purpose   : Object holding one probe                               // 
//    Time-stamp: <Fri Mar/05/2004 22:02 MET Coder@ReallySoft.de>        // 
//                                                                       // 
//                                                                       // 
//  Coded by Ralf Westram (coder@reallysoft.de) in March 2004            // 
//  Copyright Department of Microbiology (Technical University Munich)   // 
//                                                                       // 
//  Visit our web site at: http://www.arb-home.de/                       // 
//                                                                       // 
//  ==================================================================== // 


public class Probe
{
    private String  probeString;
    private String  group_id;
    private boolean isexact = false;

    private int    temperature = -1;
    private double gc_content  = -1;
    private int    no_of_hits  = -1;

    public static GroupCache groupCache = null;

    //     public Probe(String probeString, String group_id) {
//         this.probeString = probeString;
//         this.group_id    = group_id;
//     }

    public Probe(String probe_definition, TreeNode target) throws Exception {
        System.out.println("probe_definition='"+probe_definition+"'");

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

    public String getDisplayString() {
        return group_id+" "+probeString;
    }
    
    public static String getError() {
        return groupCache.getError();
    }
}
