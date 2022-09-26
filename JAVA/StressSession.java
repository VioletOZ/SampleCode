package server.stresser;

import org.apache.http.NameValuePair;
import org.apache.http.client.HttpClient;
import org.apache.http.client.ResponseHandler;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.utils.URLEncodedUtils;
import org.apache.http.conn.ClientConnectionManager;
import org.apache.http.impl.client.BasicResponseHandler;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.impl.conn.tsccm.ThreadSafeClientConnManager;
import org.apache.http.message.BasicNameValuePair;
import org.apache.http.params.HttpParams;
import org.json.JSONArray;
import org.json.JSONObject;
import server.StresserConfig;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentSkipListMap;

public class ServerManager {
    public class Server {
        public int id;
        public String host;
        public String name;

        public Server(int id, String host, String name) {
            this.id = id;
            this.host = host;
            this.name = name;
        }
    }

    private static final ServerManager singleton = new ServerManager();
    public static ServerManager get() { return singleton; }

    private static HttpClient client = null;
    private ConcurrentSkipListMap<Integer, Server> servers = new ConcurrentSkipListMap<>();

    public void init() {
        //servers.php로 부터 현재 stresser group id에 해당하는 group의 서버 내의 모든 채널 정보를 갖고 옴

        client = getThreadSafeClient();

        try {
            List<NameValuePair> params = new ArrayList<>();
            params.add(new BasicNameValuePair("group_id", "" + StresserConfig.stresserGroupID));

            HttpGet get = new HttpGet(StresserConfig.webURL + "/servers.php?" + URLEncodedUtils.format(params, "UTF-8"));
            ResponseHandler<String> rh = new BasicResponseHandler();
            JSONObject data = new JSONObject(client.execute(get, rh));

            // response로 받아 온 해당 서버(채널)들이 켜져있다고 가정한다.
            JSONArray jsonArray = data.getJSONArray("servers");
            for (int i = 0; i < jsonArray.length(); i++) {
                JSONObject jsonObject = jsonArray.getJSONObject(i);

                Server server = new Server(jsonObject.getInt("id"), jsonObject.getString("host"), jsonObject.getString("name"));
                servers.put(jsonObject.getInt("id"), server);
            }

            if(data.getInt("status") < 0)
                System.out.println("Failed to updateItem server info. status=" + data.getInt("status"));

        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    public Server getServer() {
        //servers.php로 부터 모든 서버 정보를 갖고 온 뒤 현재 stresser group id 에 해당하는 server만 리턴

        client = getThreadSafeClient();

        try {
            HttpGet get = new HttpGet(StresserConfig.webURL + "/servers.php?group_id=" + StresserConfig.stresserGroupID);
            ResponseHandler<String> rh = new BasicResponseHandler();
            JSONObject data = new JSONObject(client.execute(get, rh));

            // response로 받아 온 해당 서버(채널)들이 켜져있다고 가정한다.
            JSONArray jsonArray = data.getJSONArray("servers");
            Server server;

            for (int i = 0; i < jsonArray.length(); i++) {
                JSONObject jsonObject = jsonArray.getJSONObject(i);

                if (jsonObject.getInt("id") == StresserConfig.stresserGroupID) {
                    server = new Server(jsonObject.getInt("id"), jsonObject.getString("host"), jsonObject.getString("name"));
                    return server;
                }
            }

            if(data.getInt("status") < 0)
                System.out.println("Failed to updateItem server info. status=" + data.getInt("status"));

        } catch (Exception ex) {
            ex.printStackTrace();
        }
        return null;
    }

    private HttpClient getThreadSafeClient()  {
        if (client != null)
            return client;
        client = new DefaultHttpClient();
        ClientConnectionManager mgr = client.getConnectionManager();
        HttpParams params = client.getParams();
        client = new DefaultHttpClient(new ThreadSafeClientConnManager(params, mgr.getSchemeRegistry()), params);
        return client;
    }

    public ConcurrentSkipListMap<Integer, Server> getServers() {
        return this.servers;
    }
}
