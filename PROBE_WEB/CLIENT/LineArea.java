public class LineArea
{
    private int[] corners;

    public LineArea(int x1, int y1, int x2, int y2)
    {
        corners = new int[4];
        corners[0] = x1;
        corners[1] = y1;
        corners[2] = x2;
        corners[3] = y2;
    }

    public LineArea(int [] coordinates)
    {
        corners = coordinates;
    }

    public boolean isInside(int x, int y, int tolerance)
    {
        return ((x+tolerance) >= corners[0] && (x-tolerance) <= corners[2] &&
                (y+tolerance) >= corners[1] && (y-tolerance) <= corners[3]);
    }

    public boolean isAtLeft(int x, int y, int tolerance)
    {
        // assumes that 0/1 is the left corner!
        return ((x+tolerance) >= corners[0] && (x-tolerance) <= corners[0] &&
                (y+tolerance) >= corners[1] && (y-tolerance) <= corners[1]);
    }


    public void getBorder()
    {
        System.out.println("Click found in area: " + corners[0] + "/" + corners[1] + "\t"
                           + corners[2] + "/" + corners[3]);
    }

}
