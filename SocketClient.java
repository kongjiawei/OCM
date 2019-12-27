import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.*;
import java.lang.String;
import java.net.UnknownHostException;
import java.io.*;


public class SocketClient {
    public static final String IP_ADDR = "192.168.108.25"; //address of server
    public static int PORT = 8888; //port of server

    public static double bytes2Double(byte[] arr, int k){
        long value=0;
        for(int i=0;i< 8;i++){
            value|=((long)(arr[k]&0xff))<<(8*i);
            k++;
        }
        return Double.longBitsToDouble(value);
    }

    public static void main(String args[]){
        Socket socket = null;
        try{
            socket = new Socket(IP_ADDR, PORT);
            // send data to server
            DataOutputStream out = new DataOutputStream(socket.getOutputStream());
            //System.out.print("please input:");
            String str = "193.1 0.3";
            //String str = new BufferedReader(new InputStreamReader(System.in)).readLine();
            out.write(str.getBytes());

            //read data from server
            byte []receive = new byte[1024];

//            socket.getInputStream().read(receive);
//            System.out.print("receive");
//            System.out.print(receive);
//            System.out.print('\n');
//            String receive_string = new String(receive);
//            System.out.print("receive_string");
//            System.out.print(receive_string);
//            System.out.print('\n');
//            double receive_double = Double.parseDouble(receive_string);
            int len = socket.getInputStream().read(receive);
            double[] value = new double[len / 8];
//            System.out.print("len:"+len);
//            String ret = new String(receive,1,8);
//            System.out.println("data send from client:" + ret + '\n');
            for(int i = 0, k = 0; i < len; i = i + 8, k++) {
                value[k] = bytes2Double(receive, i);
                System.out.print(value[k]);
                System.out.print('\n');
            }

            System.out.print("receive");

            System.out.print('\n');
            while(true);
            //input.close();
        } catch (Exception e){
            System.out.println("exception:" + e.getMessage());
        }finally{
//            if(socket != null){
//                try{
//                    socket.close();
//                }catch(IOException e){
//                    socket = null;
//                    System.out.println("finally exception:" + e.getMessage());
//                }
//            }


        }






//          System.out.print("hello");
//        long startTime,endTime;
//        startTime = System.currentTimeMillis();
//        System.out.println("startTime:"+startTime);
//
//        Thread.sleep(5000);
//        endTime = System.currentTimeMillis();
//        System.out.println("endTime:"+endTime);
//        System.out.println("duringTime:"+(endTime - startTime));


//        startTime = fromDateStringToLong(new SimpleDateFormat("yyyy-MM-dd hh:mm:ss:SSS").format(new Date()));
    }
}