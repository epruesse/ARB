
import java.util.*;

class GroupCache
{
private HashMap groups;
private String  error;

public GroupCache()
    {
        groups = new HashMap();
        error  = null;
    }

private String retrieveMemberList(HttpSubsystem webAccess, String groupId, int probe_length) throws Exception 
    {
        String answer = webAccess.retrieveGroupMembers(groupId, probe_length);
        if (answer == null) {
            error = webAccess.getLastRequestError();
            return null;
        }

        ServerAnswer parsed = new ServerAnswer(answer, true, false);
        if (parsed.hasError()) {
            error = parsed.getError();
            return null;
        }

        if (!parsed.hasKey("membercount")) {
            error = "'membercount' expected in server answer";
            return null;
        }

        StringBuffer memberListBuffer      = new StringBuffer();
        //         String       memberList = null;
        int          membercount           = Integer.parseInt(parsed.getValue("membercount"));

        if (membercount<1) {
            error = "No members found (shouldn't occur!)";            
        }
        else {
            for (int m = 1; m <= membercount; m++) {
                String keyName = "m"+m;
                if (!parsed.hasKey(keyName)) {
                    error = "'"+keyName+"' expected in server answer";
                    return null;
                }

                String speciesName = parsed.getValue(keyName);
                if (m > 1) memberListBuffer.append(",");
                memberListBuffer.append(speciesName);

                //             if (memberList == null) {
                //                 memberList = speciesName;
                //             }
                //             else {
                //                 memberList = memberList+","+speciesName;
                //             }
            }
        }
        String memberList = memberListBuffer.toString();

        if (memberList == null) {
            error = "No members found (shouldn't occur!)";
        }

        return memberList;
    }

public String getGroupMembers(HttpSubsystem webAccess, String groupId, int probe_length) throws Exception 
    {
        error           = null;
        String hash_key = groupId+"_"+probe_length;

        // System.out.println("hash_key='"+hash_key+"'");

        String members = null;
        if (groups.containsKey(hash_key)) {
            members = (String)groups.get(hash_key);
            // System.out.println("cached: '"+members+"'");
        }
        else {
            members = retrieveMemberList(webAccess, groupId, probe_length);
            // System.out.println("retrieved: '"+members+"'");
            if (error == null) {
                groups.put(hash_key, members);
            }
        }
        return members;
    }

public String getError()
    {
        return error;
    }
}
