package com.tutpro.baresip

import android.content.Context
import android.content.res.AssetManager
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.net.InetAddress
import java.net.NetworkInterface
import java.util.Collections
import java.util.Locale
import org.json.JSONObject
import org.json.JSONException

class MicroServiceCommandProcessor(parent: MainActivity): Thread() {

    var PHONE_DEVICE_ID = 0xEA
    var PHONE_DEVICE = "phone.json"

    private var m_parent: MainActivity = parent

    private lateinit var m_asset_manager: AssetManager
    private var m_device_path: HashMap<Int, String> = HashMap<Int, String>()

    companion object {
        var libraryLoaded = false
    }

    init {
        var addr: String = "";

        if (!MicroServiceCommandProcessor.libraryLoaded) {
            Log.w(TAG, "Loading microdeviceservice library");
            System.loadLibrary("microdeviceservice");
            MicroServiceCommandProcessor.libraryLoaded = true;
            Log.w(TAG, "Loaded microdeviceservice library");

            cppInitMicroService();

            cppClearMyAddress();
            addr = getIPAddress(true);
            Log.w(TAG, "get my adr: " + addr);
            cppAddMyAddress(addr);
        }
    }

    fun getIPAddress(useIPv4: Boolean): String {
        try {
            val interfaces: List<NetworkInterface> = Collections.list(NetworkInterface.getNetworkInterfaces())
            for (intf in interfaces) {
                val addrs: List<InetAddress> = Collections.list(intf.inetAddresses)
                for (addr in addrs) {
                    if (!addr.isLoopbackAddress) {
                        val sAddr = addr.hostAddress
                        //boolean isIPv4 = InetAddressUtils.isIPv4Address(sAddr);
                        val isIPv4 = sAddr.indexOf(':') < 0
                        if (useIPv4) {
                            if (isIPv4) return sAddr
                        } else {
                            if (!isIPv4) {
                                val delim = sAddr.indexOf('%') // drop ip6 zone suffix
                                return if (delim < 0) sAddr.uppercase(Locale.getDefault()) else sAddr.substring(
                                    0,
                                    delim
                                ).uppercase(
                                    Locale.getDefault()
                                )
                            }
                        }
                    }
                }
            }
        } catch (ignored: java.lang.Exception) {
        } // for now eat exceptions
        return ""
    }

    fun init(): Int {
        m_device_path.put(PHONE_DEVICE_ID, PHONE_DEVICE)

        return 0
    }

    fun processCommand(count: Int): Int {
        var message: String = ""
        var cmd_type: String = ""
        var call_to: String = ""
        var is_found: Boolean = false

        //Log.w(TAG, "before call getCommand")
        message = cppGetCommand();
        Log.w(TAG, "after call getCommand: '$message'")
        if(message.length == 0){
            Log.w(TAG, "no message. return.")
            return 0;
        }

        try {
            val jobj = JSONObject(message);
            val jobj_params = jobj.getJSONArray("Parameters");

            cmd_type = jobj.getString("CommandType");

            if(cmd_type == "Call") {
                is_found = false
                for (i in 0 until jobj_params.length()) {
                    val jobj_param = jobj_params.getJSONObject(i);
                    val key = jobj_param.getString("Name")
                    val value = jobj_param.getString("Value")

                    Log.w(TAG, "in json: $key, $value")
                    if (key == "CallTo") {
                        call_to = value
                        is_found = true
                        break
                    }
                }
                if(is_found == true) {
                    cppSetLastCall(call_to);
                    Log.w(TAG, "before call callFromMicroService")
                    m_parent.callFromMicroService(call_to)
                    Log.w(TAG, "no message. return.")
                    Log.w(TAG, "after call callFromMicroService")
                }
            }
            else if(cmd_type == "RegisterAccount") {
                Log.w(TAG, "before call registerAccount")
                //m_parent.registerAccount()
                Log.w(TAG, "before call registerAccount")
            }
        } catch(e: JSONException){
            Log.w(TAG, "invalid json: $message")
            return -1
        }

        return 0
    }

    override fun run() {
        var count: Int = 0
        val str_json: String
        val path: String
        val input_stream: InputStream

        path = m_device_path[PHONE_DEVICE_ID].toString()
        Log.w(TAG, "start device: $PHONE_DEVICE_ID, $path")

        if (path == null) {
            Log.e(TAG, "invalid type: $PHONE_DEVICE_ID")
            return
        }

        try {
            input_stream = FileInputStream(File(path))
        } catch (e: java.lang.Exception) {
            Log.e(TAG, "file not found: $path")
            return
        }
        str_json = readStream(input_stream)

        if (cppStartDevice(PHONE_DEVICE_ID, str_json) < 0) {
            android.util.Log.i("**my", "start device failed.")
        }

        while(true){
            Thread.sleep(1000)
            processCommand(count)
            count++
        }
    }


    fun setAssetManager(asset_manager: AssetManager) {
        m_asset_manager = asset_manager
    }

    fun copyAssetFolder(context: Context, srcName: String, dstName: String): Boolean {
        Log.w(TAG, "copy directory: $srcName")

        return try {
            var result = true
            val fileList = context.assets.list(srcName) ?: return false

            if (fileList.size == 0) {
                result = copyAssetFile(context, srcName, dstName)
            } else {
                val file = File(dstName)
                result = file.mkdirs()
                for (filename in fileList) {
                    if(srcName.length == 0){
                        result = result and copyAssetFolder(
                            context,
                            filename,
                            dstName + File.separator + filename
                        )
                    }
                    else {
                        result = result and copyAssetFolder(
                            context,
                            srcName + File.separator + filename,
                            dstName + File.separator + filename
                        )
                    }
                }
            }
            result
        } catch (e: IOException) {
            e.printStackTrace()
            false
        }
    }

    fun copyAssetFile(context: Context, srcName: String, dstName: String?): Boolean {
        Log.w(TAG, "copy file: $srcName")
        return try {
            val `in` = context.assets.open(srcName)
            val outFile = File(dstName)
            val out: OutputStream = FileOutputStream(outFile)
            val buffer = ByteArray(1024)
            var read: Int
            while (`in`.read(buffer).also { read = it } != -1) {
                out.write(buffer, 0, read)
            }
            `in`.close()
            out.close()
            true
        } catch (e: IOException) {
            e.printStackTrace()
            false
        }
    }

    private fun readStream(input_stream: InputStream): String {
        val length: Int
        val buffer: ByteArray
        var str: String = ""

        try {
            length = input_stream.available()
            buffer = ByteArray(length)
            input_stream.read(buffer, 0, length)
            input_stream.close()
            str = String(buffer)
        } catch (e: Exception) {
            return ""
        }

        return str
    }

    private fun writeString(str: String, path: String): Int {
        val file: File
        val bytes: ByteArray
        file = File(path)
        bytes = str.toByteArray()
        try {
            val outputStream = FileOutputStream(file)
            outputStream.write(bytes)
            outputStream.close()
        } catch (e: Exception) {
            return -1
        }
        return 0
    }

    fun writeString(str: String, type: Int): Int {
        val file: File
        val path: String
        val bytes: ByteArray
        path = m_device_path[type].toString()
        if (path == null) {
            Log.e(TAG, "invalid type: $type")
            return -1
        }
        file = File(path)
        bytes = str.toByteArray()
        try {
            val outputStream = FileOutputStream(file)
            outputStream.write(bytes)
            outputStream.close()
        } catch (e: Exception) {
            return -1
        }
        return 0
    }

    fun setBaseDir(base_path: String) {
        var input_stream: InputStream
        var str_json: String = ""
        var path: String
        var asset_name: String = ""
        var file: File
        val key_iter: Iterator<Int>
        var key: Int

        Log.w(TAG, "set base path: $base_path")
        key_iter = m_device_path?.keys?.iterator() ?: return

        while (key_iter.hasNext()) {
            key = key_iter.next()
            asset_name = m_device_path[key].toString()
            if (asset_name == null) {
                continue
            }
            path = "$base_path/$asset_name"
            /*
            file = File(path)
            file.delete()
            if (file.exists() == false) {
                str_json = ""
                try {
                    input_stream = m_asset_manager.open(asset_name) ?: return
                    str_json = readStream(input_stream)
                } catch (e: Exception) {
                    Log.e(TAG, "asset not found: $asset_name")
                }
                Log.e(TAG, "copy file: '$path'")
                writeString(str_json, path)
            }
            */
            m_device_path.put(key, path)
        }

        cppSetBaesFolder(base_path)
        return
    }

    fun getJsonString(type: Int): String {
        val path: String
        val str_json: String?
        val input_stream: InputStream

        path = m_device_path[type].toString()
        Log.e(TAG, "start device: $type, $path")
        if (path == null) {
            Log.e(TAG, "invalid type: $type")
            return ""
        }
        input_stream = try {
            FileInputStream(File(path))
        } catch (e: Exception) {
            android.util.Log.i("**my", "file not found: $path")
            Log.e(TAG, "file not found: $path")
            return ""
        }
        str_json = readStream(input_stream)

        return str_json
    }

    //donghee
    external fun cppAddMyAddress(addr: String)
    external fun cppClearMyAddress()
    external fun cppInitMicroService(): Int
    external fun cppGetCommand(): String
    external fun cppSetBaesFolder(path: String): Int
    external fun cppStartDevice(type: Int, str_json: String): Int
    external fun cppSetLastCall(last_call: String)
    //end of donghee
}

