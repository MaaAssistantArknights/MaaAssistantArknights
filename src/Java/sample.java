import com.meo.asst.libmaa;

public class sample {
    public static void main(String[] args) {
        long handle = libmaa.create("D:\\Code\\MeoAssistantArknights\\x64\\Release");
        libmaa.destroy(handle);
    }
}
