package bgu.spl.net.srv;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;

public class ConnectionsImpl<T> implements Connections<T>  {

    private final Map<Integer, ConnectionHandler<T>> handlers = new ConcurrentHashMap<>();
    private final Map<String, CopyOnWriteArrayList<Integer>> channels = new ConcurrentHashMap<>();    
    private final Map<Integer, Map<Integer, String>> subscriptionsChannel = new ConcurrentHashMap<>();
    

    public boolean send(int connectionId, T msg){
        ConnectionHandler<T> handler = null;
        if(handlers.containsKey(connectionId)){
            handler = handlers.get(connectionId);
        }
        
        if (handler == null) {
            return false;
        }
        handler.send(msg);
        return true;
    }
    public void send(String channel, T msg){
        CopyOnWriteArrayList<Integer> list = null;
        if(channels.containsKey(channel)){
            list = new CopyOnWriteArrayList<>(channels.get(channel));
        }
        if(list != null){
            
            for(Integer connectionId : list){
                send(connectionId, msg);
            }
                
        }
    }
    public void disconnect(int connectionId){
        handlers.remove(connectionId);
        Map<Integer, String> subscriptions = subscriptionsChannel.remove(connectionId);
        if (subscriptions != null) {
            for (String channel : subscriptions.values()) {
                CopyOnWriteArrayList<Integer> subscribers = channels.get(channel);
                if (subscribers != null) {
                    subscribers.remove(Integer.valueOf(connectionId));
                }
            }
        }
    }
    
    public void subscribe (String curChannel,int connectionId, int subscriptionId){
       // Ensure thread-safe addition to the channel's subscriber list if thread A and B try to subscribe 
       // to the same new channel simultaneously thay both will enter here and create a list and one will override the other
       synchronized (channels) {
            if (!channels.containsKey(curChannel)) {
                channels.put(curChannel, new CopyOnWriteArrayList<>());
            }
        }
        channels.get(curChannel).add(connectionId);
        Map<Integer, String> userSubscriptions = subscriptionsChannel.get(connectionId);
        if (userSubscriptions != null) {
            subscriptionsChannel.get(connectionId).put(subscriptionId,curChannel);
        }
        
    }
    public boolean isSubscribed(int connectionId , String channel){
        if(channels.containsKey(channel)){
            return channels.get(channel).contains(connectionId);
        }
        return false;
    }
    public void unsubscribe (int connectionId , int subscriptionId){
       Map<Integer, String> userSubs = subscriptionsChannel.get(connectionId);
        
        if (userSubs != null) {
            String channel = userSubs.remove(subscriptionId);
            if (channel != null) {
                CopyOnWriteArrayList<Integer> subscribers = channels.get(channel);
                if (subscribers != null) {
                    subscribers.remove(Integer.valueOf(connectionId));
                }
            }
        }
        
    }
    public void addConnection(int connectionId, ConnectionHandler<T> handler) {
        handlers.put(connectionId, handler);
        subscriptionsChannel.put(connectionId, new ConcurrentHashMap<>());
    }
    public int getSubscriptionId(int connectionId, String channelName) {
        Map<Integer, String> subs = subscriptionsChannel.get(connectionId);
        if (subs != null) {
            for (Integer id : subs.keySet()) {
                
                if (subs.get(id).equals(channelName)) {
                    return id;
                }
            }
        }
        return -1;
    }
    public Map<String, CopyOnWriteArrayList<Integer>> getChannels() {
        return channels;
    }
    
}

