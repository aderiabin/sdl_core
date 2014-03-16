package com.ford.syncV4.protocol.secure.secureproxy;

import android.util.Log;

import com.ford.syncV4.protocol.enums.ServiceType;
import com.ford.syncV4.transport.ITransportListener;
import com.ford.syncV4.util.BitConverter;
import com.ford.syncV4.util.DebugTool;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.CountDownLatch;

import javax.net.ssl.HandshakeCompletedEvent;
import javax.net.ssl.HandshakeCompletedListener;

/**
 * Created by Andrew Batutin on 3/12/14.
 */
public class ProtocolSecureManager {

    private IHandshakeDataListener handshakeDataListener;
    private ISecureProxyServer listener;
    SecureProxyServer secureProxyServer;
    SSLClient sslClient;
    private boolean handshakeFinished;
    private final CountDownLatch countDownLatchInput = new CountDownLatch(1);
    private final CountDownLatch countDownLatchOutput = new CountDownLatch(1);
    private byte[] cypheredData = null;
    private byte[] deCypheredData = null;
    Set<ServiceType> serviceTypesToEncrypt = new HashSet<ServiceType>();

    public void addServiceToEncrypt(ServiceType serviceType) {
        serviceTypesToEncrypt.add(serviceType);
    }

    public void removeServiceTypeToEncrypt(ServiceType serviceType) {
        serviceTypesToEncrypt.remove(serviceType);
    }

    public boolean containsServiceTypeToEncrypt(ServiceType serviceType) {
        return serviceTypesToEncrypt.contains(serviceType);
    }

    public ProtocolSecureManager(IHandshakeDataListener listener) {
        this.handshakeDataListener = listener;
    }

    public void startHandShake() {
        startSSLClient();
    }

    public void setupSecureEnvironment() {
        startSecureProxy();
    }

    private void startSecureProxy() {
        secureProxyServer = new SecureProxyServer(new ISecureProxyServer() {
            @Override
            public void onDataReceived(byte[] data) {
                if (!handshakeFinished) {
                    handshakeDataListener.onHandshakeDataReceived(data);
                }
            }
        },
                new ITransportListener() {
                    @Override
                    public void onTransportBytesReceived(byte[] receivedBytes, int receivedBytesLength) {

                    }

                    @Override
                    public void onTransportConnected() {

                    }

                    @Override
                    public void onTransportDisconnected(String info) {

                    }

                    @Override
                    public void onTransportError(String info, Exception e) {
                        reportAnError(e);
                    }

                    @Override
                    public void onServerSocketInit(int serverSocketPort) {

                    }
                }
        );
        try {
            secureProxyServer.setupServer();
        } catch (IOException e) {
            DebugTool.logError("Failed to setup secureProxyServer", e);
        }
    }

    private void startSSLClient() {
        sslClient = new SSLClient(new ITransportListener() {
            @Override
            public void onTransportBytesReceived(byte[] receivedBytes, int receivedBytesLength) {

            }

            @Override
            public void onTransportConnected() {
            }

            @Override
            public void onTransportDisconnected(String info) {

            }

            @Override
            public void onTransportError(String info, Exception e) {
                reportAnError(e);
            }

            @Override
            public void onServerSocketInit(int serverSocketPort) {

            }
        }, new HandshakeCompletedListener() {
            @Override
            public void handshakeCompleted(HandshakeCompletedEvent event) {
                setHandshakeFinished(true);
                handshakeDataListener.onHandShakeCompleted();
                DebugTool.logInfo("GREAT SUCCESS" + event.toString());
            }
        });
        try {
            sslClient.setupClient();
        } catch (IOException e) {
            DebugTool.logError("Failed to setup sslClient", e);
        }
    }


    public synchronized byte[] sendDataToProxyServer(ServiceType serviceType, byte[] data) throws IOException, InterruptedException {
        if (serviceTypesToEncrypt.contains(serviceType)) {
            writeDataToProxyServer(data, new IRCCodedDataListener() {
                @Override
                public void onRPCPayloadCoded(byte[] bytes) {
                    if (handshakeFinished) {
                        deCypheredData = Arrays.copyOf(bytes, bytes.length);
                        countDownLatchOutput.countDown();
                    }
                }
            });
            countDownLatchOutput.await();
            return deCypheredData;
        } else {
            return data;
        }
    }


    public byte[] sendDataToProxyServerByteByByte(ServiceType serviceType, byte[] data) throws IOException, InterruptedException {
        if (serviceTypesToEncrypt.contains(serviceType)) {
            IRCCodedDataListener listenerOfDeCodedData = new RPCDeCodedDataListener();
            writeDataToProxyServer(data, listenerOfDeCodedData);
            countDownLatchOutput.await();
            return deCypheredData;
        } else {
            return data;
        }
    }


    public synchronized void writeDataToProxyServer(byte[] data, IRCCodedDataListener ircCodedDataListener) throws IOException {
        sslClient.setRPCPacketListener(ircCodedDataListener);
        secureProxyServer.writeData(data);

    }

    public synchronized void writeDataToSSLSocket(byte[] data, IRCCodedDataListener ircCodedDataListener) throws IOException {
        secureProxyServer.setRPCPacketListener(ircCodedDataListener);
        sslClient.writeData(data);
    }


    private synchronized void setHandshakeFinished(boolean handshakeFinished) {
        this.handshakeFinished = handshakeFinished;
    }

    IRCCodedDataListener listenerOFCodedData = new IRCCodedDataListener() {
        @Override
        public void onRPCPayloadCoded(byte[] bytes) {
            if (handshakeFinished) {
                cypheredData = bytes;
                if (cypheredData != null) {
                    Log.i("cypheredData.length < 1000", " onRPCPayloadCoded " + bytes.length + " cypheredData.l " + cypheredData.length + " thread:" + Thread.currentThread().getName());
                }
                countDownLatchInput.countDown();
                if (cypheredData != null) {
                    Log.i("cypheredData.length < 1000", " countDownLatchInput.countDown(); " + bytes.length + " cypheredData.l " + cypheredData.length + " thread:" + Thread.currentThread().getName());
                }
            }
        }
    };

    public synchronized byte[] sendDataTOSSLClient(ServiceType serviceType, byte[] data) throws IOException, InterruptedException {
        if (serviceTypesToEncrypt.contains(serviceType)) {
            String msg = "Start writeDataToSSLSocket with data " + data.length + " thread:" + Thread.currentThread().getName();
            Log.i("cypheredData.length < 1000", msg);
            writeDataToSSLSocket(data, listenerOFCodedData);
            if (cypheredData != null) {
                Log.i("cypheredData.length < 1000", "Stop writeDataToSSLSocket with data " + data.length + " cypheredData.l " + cypheredData.length + " thread:" + Thread.currentThread().getName());
            }
            Log.i("cypheredData.length < 1000", "countDownLatchInput.await();");
            countDownLatchInput.await();
            if (cypheredData != null) {
                Log.i("cypheredData.length < 1000", " countDownLatchInput.await() " + data.length + " cypheredData.l " + cypheredData.length + " thread:" + Thread.currentThread().getName());
            }
            return cypheredData;
        } else {
            return data;
        }
    }

    public void reportAnError(Exception e) {
        handshakeDataListener.onError(e);
    }

    private class RPCDeCodedDataListener implements IRCCodedDataListener {

        ByteArrayOutputStream bOutput = new ByteArrayOutputStream();

        final int bytesToExpectForHeader = 32;
        int bytesReceived = 0;
        int jsonSize = 0;

        @Override
        public void onRPCPayloadCoded(byte[] bytes) {
            if (handshakeFinished) {

                try {
                    bOutput.write(bytes);
                    bytesReceived += bytes.length;
                } catch (IOException e) {
                    DebugTool.logError("Failed to parse frame", e);
                } finally {
                    countDownLatchOutput.countDown();
                }
                if (bytesReceived > bytesToExpectForHeader) {
                    jsonSize = getJsonSizeFromHeader(Arrays.copyOfRange(bOutput.toByteArray(), 0, 32));
                }

                if (jsonSize != 0 && bOutput.size() - 12 == jsonSize) {
                    deCypheredData = bOutput.toByteArray();
                    countDownLatchOutput.countDown();
                    bytesReceived = 0;
                    jsonSize = 0;
                }
            }
        }

        private int getJsonSizeFromHeader(byte[] bytes) {
            int jsonSize = BitConverter.intFromByteArray(bytes, 8);
            return jsonSize;
        }
    }
}
