// creates line area objects which handle rectangle borders up to 2 exp 31

public class LineAreaFactory
{
private int halo;

public LineAreaFactory(int a)
    {
        halo = a;

    }

public LineArea getLAObject(int x1, int y1, int x2, int y2)
    {
        return new LineArea(
                            (x1 - halo) < 0 ? 0 : x1 - halo,
                            (y1 - halo) < 0 ? 0 : y1 - halo,
                            x2,
                            y2); 
    }

public LineArea getLAObject(int line [])
    {
        return new LineArea(line); 
    }


}
