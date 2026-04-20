package bgu.spl.net.srv;

import java.io.IOException;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;

public interface Connections<T> {

    boolean send(int connectionId, T msg);

    void send(String channel, T msg);

    void disconnect(int connectionId);
    boolean isSubscribed(int connectionId , String channel );
    void subscribe (String curChannel,int connectionId, int subscriptionId);
    void unsubscribe (int connectionId , int subscriptionId);
    Map<String, CopyOnWriteArrayList<Integer>> getChannels();
    int getSubscriptionId(int connectionId, String channelName);

 

}
