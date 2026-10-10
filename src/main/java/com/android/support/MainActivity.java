package com.android.support;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.app.ProgressDialog;
import android.net.Uri;
import android.provider.Settings;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;

public class MainActivity extends Activity {


	public static String libname = "libV2.so"; 
	public static String downloadurl = "https://xlreyt.x10.mx/Onlinelibxlreyt/libV2.so"; 

	public String GameActivity = "com.tencent.tmgp.cod.CODMainActivity";

	/** Native module built by src/main/jni/Android.mk -> libgspace1.so */
	public static final String LOCAL_LIBRARY = "gspace1";

	/** Loads the module that ships inside this APK (built from the local sources). */
	private boolean loadBundledLibrary() {
		try {
			System.loadLibrary(LOCAL_LIBRARY);
			Log.i("ModLoader", "loaded bundled lib" + LOCAL_LIBRARY + ".so (local build)");
			return true;
		} catch (Throwable t) {
			Log.e("ModLoader", "bundled lib" + LOCAL_LIBRARY + ".so unavailable: " + t);
			return false;
		}
	}

	public boolean hasLaunched = false;
	static ProgressDialog progressDialog;
	static long serverLastModified;
	static int fileLength = 0;
	static int currentProgress = 0;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);


		if (hasLaunched) {
			return;
		}

		hasLaunched = true;

		if (Build.VERSION.SDK_INT >= 23) {
			if (!Settings.canDrawOverlays(this)) {
				Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + this.getPackageName()));
				this.startActivity(intent);
			}
		}


		progressDialog = new ProgressDialog(this);
		progressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
		progressDialog.setMessage("Downloading...");
		progressDialog.setCancelable(false);

	
		String savepath = this.getFilesDir().getAbsolutePath() + "/" + libname;

		// Load the library built from THIS project first (Android.mk module "gspace1",
		// packaged in the APK as libgspace1.so). The previous flow only ever loaded a
		// prebuilt library fetched from a server, so source changes (menu UI, login
		// page, skins) never appeared in the running mod.
		if (loadBundledLibrary()) {
			startGame();
			return;
		}

		// Fallback: legacy remote download, used only when the local module is not
		// packaged in this APK.
		new FileDownloadTask().execute(downloadurl, savepath);
    }

   
	private class FileDownloadTask extends AsyncTask<String, Integer, Boolean> {

        @Override
		protected void onPreExecute() {
            super.onPreExecute();
			progressDialog.show();
		}

		@Override
        protected Boolean doInBackground(String... strings) {
			String fileURL = strings[0];
			String savePath = strings[1];

			try {

				URL url = new URL(fileURL);
				HttpURLConnection httpConn = (HttpURLConnection) url.openConnection();
				int responseCode = httpConn.getResponseCode();

                if (responseCode == HttpURLConnection.HTTP_OK) {
                    serverLastModified = httpConn.getLastModified();
                    long storedLastModified = getLastModifiedTimeFromPrefs();

                    fileLength = httpConn.getContentLength();
                    byte[] buffer = new byte[4096];
                    int bytesRead;
                    int totalBytesRead = 0;


                    if (serverLastModified != storedLastModified) {
                       
                        File directory = new File(savePath);
                        if (directory.exists() && directory.isDirectory()) {

                            File[] files = directory.listFiles();
                            for (File fl : files) {
                                if (fl.getName().endsWith(".so")) {
                                    fl.delete();
                                }
                            }
                        }

                        
                        try (InputStream inputStream = httpConn.getInputStream();
                        FileOutputStream outputStream = new FileOutputStream(savePath)) {

                            while ((bytesRead = inputStream.read(buffer)) != -1) {
                                outputStream.write(buffer, 0, bytesRead);
                                totalBytesRead += bytesRead;
                                
                                currentProgress = (int) ((totalBytesRead * 100) / fileLength);
                                publishProgress(currentProgress);
                            }
                        }

                        return true; 
                    } else {
                    
                        progressDialog.setMessage("Already up to date");
						// Thread.sleep(3000);
                        progressDialog.setProgress(100);
                        return false; 
                    }
                } else {
                    Log.e("Download", "Failed to download file. Server replied HTTP code: " + responseCode);
                }
                httpConn.disconnect();
            } catch (IOException e) {
                Log.e("Download", "Download error", e);
            }
            return false; 
        }

        @Override
        protected void onProgressUpdate(Integer... values) {
            super.onProgressUpdate(values);
            progressDialog.setMax(100);
            progressDialog.setProgress(values[0]);
            progressDialog.setMessage("Downloading... ");
        }

        @Override
        protected void onPostExecute(Boolean result) {
            progressDialog.dismiss();

            File libFile = new File(MainActivity.this.getFilesDir().getAbsolutePath() + "/" + libname);

            // Always prefer the library built with this project. Loading the cached
            // prebuilt copy here is what made the app run a stale mod and hid every
            // source change.
            if (loadBundledLibrary()) {
                if (result) {
                    saveLastModifiedTime(serverLastModified);
                }
                startGame();
                return;
            }

            // Legacy fallback: only reachable when this APK has no local module.
            if (result) {

                saveLastModifiedTime(serverLastModified);
                if (libFile.exists()) {

                    System.load(libFile.getAbsolutePath());
                    startGame();
                }
            } else {

                if (libFile.exists()) {
                    System.load(libFile.getAbsolutePath());
                    startGame();
                }
            }
        }

       
        private long getLastModifiedTimeFromPrefs() {
            return getSharedPreferences("library_prefs", MODE_PRIVATE).getLong("last_modified", 0);
        }

        
        private void saveLastModifiedTime(long lastModified) {
            getSharedPreferences("library_prefs", MODE_PRIVATE).edit()
                .putLong("last_modified", lastModified)
                .apply();
        }
    }

    
    private void startGame() {
        try {
           
            Intent intent = new Intent(MainActivity.this, Class.forName(GameActivity));
            startActivity(intent);
        } catch (ClassNotFoundException e) {
            Log.e("StartGame", "Game activity not found", e);
        }
    }
}
