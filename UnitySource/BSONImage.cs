using System.Collections;
using System.Collections.Generic;
using UnityEngine;


public class BsonImage
{
    public string viewGroup { get; set; }
    public string viewType { get; set; }
    public string viewId { get; set; }
    public string seqNum { get; set; }
    public string mimeType { get; set; }
    public int width { get; set; }
    public int height { get; set; }
    public byte[] imageData { get; set; }

    public long frameTime { get; set; }
    public double elapsedTime { get; set; }
    public double MBPS { get; set; }
    public double FPS { get; set; }

    public override string ToString()
    {
        string retString = "";

        retString += "viewGroup: " + viewGroup;
        retString += "\nviewType: " + viewType;
        retString += "\nviewId: " + viewId;
        retString += "\nseqNum: " + seqNum;
        retString += "\nmimeType: " + mimeType;
        retString += "\nwidth: " + width.ToString();
        retString += "\nheight: " + height.ToString();
        retString += "\nimageData Length: " + imageData.Length;
        retString += "\nframeTime: " + frameTime;
        retString += "\nelapsedTime: " + elapsedTime;
        retString += "\nMBPS: " + MBPS;
        retString += "\nFPS: " + FPS;

        return retString;
    }
}
