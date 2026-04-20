package bgu.spl.net.impl.stomp;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.impl.data.Database;
import bgu.spl.net.impl.data.LoginStatus;
import bgu.spl.net.srv.Connections;


public class StompMessagingProtocolImpl implements StompMessagingProtocol<String> {
    private int connectionId;
    private Connections<String> connections;
    private boolean shouldTerminate = false;
    private boolean isLoggedIn = false;
    private static AtomicInteger messageIdCounter = new AtomicInteger(0);

    
    @Override
    public void start(int connectionId, Connections<String> connections){
        this.connectionId = connectionId;
        this.connections = connections;
    }
    @Override
    public void process(String msg) {
        if (msg == null || msg.trim().isEmpty()) {
                return;
        }
        try {
            String[] parts = msg.split("\n\n", 2);
            String headerSection = parts[0];
            String body = "";
            if (parts.length > 1) {
                body = parts[1];
            }

            String[] lines = headerSection.split("\n");
            String command = lines[0];
            Map<String, String> headers = new HashMap<>();

            for (int i = 1; i < lines.length; i++) {
                String[] topicValue = lines[i].split(":", 2);
                if (topicValue.length == 2) {
                    headers.put(topicValue[0], topicValue[1]);
                }
            }
            System.out.println("Processing command: " + command);

            switch (command) {
                case "CONNECT":
                    handleConnect(headers, msg);
                    break;
                case "SEND":
                    handleSend(headers, body, msg);
                    break;
                case "SUBSCRIBE":
                    handleSubscribe(headers, msg);
                    break;
                case "UNSUBSCRIBE":
                    handleUnsubscribe(headers, msg);
                    break;
                case "DISCONNECT":
                    handleDisconnect(headers, msg);
                    break;
                default:
                    hendelError("Unknown command", "Command not recognized", headers, msg);
                    break;
            }
        } catch (Exception e) {
            hendelError("Server Error", "The server encountered an error: " + e.getMessage(), null, msg);
        }
    }
    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }
    private void handleConnect(Map<String, String> headers, String msg) {
        String acceptVersion = headers.get("accept-version");
        String host = headers.get("host");
        if (acceptVersion == null || !acceptVersion.equals("1.2") || host == null || !host.equals("stomp.cs.bgu.ac.il")) {
            hendelError("Malformed frame received", "Invalid accept-version or host header.", headers, msg);
            return;
        }
        String login = headers.get("login");
        String passcode = headers.get("passcode");
        LoginStatus status = Database.getInstance().login(connectionId, login, passcode);

        switch (status) {
            case LOGGED_IN_SUCCESSFULLY:
            case ADDED_NEW_USER:
                isLoggedIn = true;
                String response = "CONNECTED\nversion:1.2\n\n\u0000";
                connections.send(connectionId, response);
                break;

            case WRONG_PASSWORD:
                hendelError("Wrong password", "Password does not match the username.", headers, msg);
                break;

            case ALREADY_LOGGED_IN:
                hendelError("User already logged in", "The user is already logged in to the system.", headers, msg);
                break;
                
            case CLIENT_ALREADY_CONNECTED:
                hendelError("Client already connected", "The current connection is already associated with a user.", headers, msg);
                break;
        }

        sendReceiptIfNeeded(headers);
    }
    private void handleSend(Map<String, String> headers, String body, String msg) {
        if (!isLoggedIn) {
            hendelError("User not logged in", "Client tried to send message before logging in", headers, msg);
            return;
        }
        String destination = headers.get("destination");
        if (destination == null) {
            hendelError("Malformed frame received", "Did not contain a destination header, which is REQUIRED.", headers, msg);
            return;
        }
        if(!connections.isSubscribed(connectionId, destination)){
            hendelError("User not subscribed to destination: " + destination, "Client tried to send message to an unsubscribed destination.", headers, msg);
            return;
        }
        String file = headers.get("file");
        String user = "unknown"; 
        String[] lines = body.split("\n");
        for (String line : lines) {
            int colonIndex = line.indexOf(':');
            if (colonIndex == -1) {
                continue;
            }
            String key = line.substring(0, colonIndex).trim();
            String value = line.substring(colonIndex + 1).trim();

            if (key.equals("user")) {
                user = value;
                break;
            }
        }

         CopyOnWriteArrayList<Integer> list = null;
        if(connections.getChannels().containsKey(destination)){
            list = new CopyOnWriteArrayList<>(connections.getChannels().get(destination));
        }
        if(list != null){
            int msgId = messageIdCounter.getAndIncrement();
            for(Integer connectionId : list){
                int subId = connections.getSubscriptionId(connectionId, destination);
                String messageFrame = "MESSAGE\n" +
                                      "subscription:" + subId + "\n" + 
                                      "message-id:" + msgId + "\n" +
                                      "destination:" + destination + "\n" +
                                      "\n" +
                                      msg + 
                                      "\u0000";
                connections.send(connectionId, messageFrame);
                sendReceiptIfNeeded(headers);
            }
                
        }

        Database.getInstance().trackFileUpload(user, file, destination);
        
    
    }

    private void handleSubscribe(Map<String, String> headers, String msg) {
        if (!isLoggedIn) {
            hendelError("User not logged in", "Client tried to subscribe before logging in", headers, msg);
            return;
        }
        String destination = headers.get("destination");
        if (destination == null) {
            hendelError("Malformed frame received", "Did not contain a destination header, which is REQUIRED.", headers, msg);
            return;
        }
        String id = headers.get("id");
        if (id == null) {
            hendelError("Missing id header for SUBSCRIBE", "Did not contain an id header, which is REQUIRED.", headers, msg);
            return; 
        }
        int subscriptionId;
        try {
            subscriptionId = Integer.parseInt(id);
        } catch (NumberFormatException e) {
            hendelError("Invalid subscription ID", "Subscription ID must be an integer.", headers, msg);
            return;
        }

        connections.subscribe(destination, connectionId, subscriptionId);
        sendReceiptIfNeeded(headers);
    }
    private void handleUnsubscribe(Map<String, String> headers, String msg) {
        if (!isLoggedIn) {
            hendelError("User not logged in", "Client tried to unsubscribe before logging in", headers, msg);
            return;
        }
         String id = headers.get("id");
        if (id == null) {
            hendelError("Missing id header for UNSUBSCRIBE", "Did not contain an id header, which is REQUIRED.", headers, msg);
            return; 
        }
        int unsubscriptionId;
        try {
            unsubscriptionId = Integer.parseInt(id);
        } catch (NumberFormatException e) {
            hendelError("Invalid unsubscription ID", "Unsubscription ID must be an integer.", headers, msg);
            return;
        }

        connections.unsubscribe(connectionId, unsubscriptionId);
        sendReceiptIfNeeded(headers);
    }
    private void handleDisconnect(Map<String, String> headers, String msg) {
         if (!isLoggedIn) {
            hendelError("User not logged in", "Client tried to disconnect before logging in", headers, msg);
            return;
        }
        String receiptId = headers.get("receipt");
        if (receiptId == null) {
            hendelError("Missing receipt header for DISCONNECT", "Did not contain a receipt header, which is REQUIRED.", headers, msg);
            return;
        }
        Database.getInstance().logout(connectionId);
        sendReceiptIfNeeded(headers);
        connections.disconnect(connectionId);
        shouldTerminate = true;
        isLoggedIn = false;   
     
    }
    
    private void sendReceiptIfNeeded(Map<String, String> headers) {
        if (headers.containsKey("receipt")) {
            String receiptId = headers.get("receipt");
            String response = "RECEIPT\nreceipt-id:" + receiptId + "\n\n\u0000";
            connections.send(connectionId, response);
        }
    }

    private void hendelError(String errorHeader, String errorDescription, Map<String, String> headers, String originalMessage) {
        StringBuilder sb = new StringBuilder();
        sb.append("ERROR\n");
        if (headers != null && headers.containsKey("receipt")) {
            sb.append("receipt-id:").append(headers.get("receipt")).append("\n");
        }

        sb.append("message:").append(errorHeader).append("\n");
        sb.append("\n"); 
        sb.append("The message:\n-----\n");
        sb.append(originalMessage);
        sb.append("\n-----\n");
        sb.append(errorDescription); 
        sb.append("\u0000");
        
        connections.send(connectionId, sb.toString());
        if (isLoggedIn) {
            Database.getInstance().logout(connectionId);
        }
        connections.disconnect(connectionId);
        shouldTerminate = true;
        isLoggedIn = false;

    }

}
    


