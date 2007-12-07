// important note:
// this mouse adapter is permanently bound to its TreeDisplay, no other event sources are expected rigth now !!!


import java.awt.*;
import java.awt.event.*;

public class DisplayMouseAdapter extends MouseAdapter
{
private TreeDisplay td;
public DisplayMouseAdapter()
    {
        // nothing special to do
    }

public DisplayMouseAdapter(TreeDisplay treeDisplay)
    {
        // nothing special to do
        td = treeDisplay;
    }


public void mouseClicked(MouseEvent event)
    {
        // System.out.print("left mouse button clicked at ");
        // System.out.print("x pos:" + event.getX());
        // System.out.println("y pos:" + event.getY());

        if (td == null)
            {
                td = (TreeDisplay) event.getSource();
            }



        if(event.getModifiers() == InputEvent.BUTTON1_MASK)
            {
                td.handleLeftMouseClick(event.getX(), event.getY());
            }

        if(event.getModifiers() == InputEvent.BUTTON3_MASK)
            {
                td.handleRightMouseClick(event.getX(), event.getY());
            }



    };


}
