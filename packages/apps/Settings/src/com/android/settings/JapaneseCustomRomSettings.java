
package com.android.settings;

import com.android.settings.FileListDialog;
import android.app.ActivityManagerNative;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.backup.IBackupManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.VerifierDeviceIdentity;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Parcel;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.StrictMode;
import android.os.SystemProperties;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.preference.Preference.OnPreferenceChangeListener;
import android.provider.Settings;
import android.text.TextUtils;
import android.view.IWindowManager;
import java.io.File;
import android.widget.Toast;
import android.preference.DialogPreference;
import android.content.SharedPreferences;
import android.util.AttributeSet;
import android.view.View;
import android.util.Log;
import android.os.Environment;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import java.io.IOException;
import android.util.Log;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.nio.channels.FileChannel;
import java.io.FileOutputStream;
import android.media.AudioManager;
import android.app.WallpaperManager;
import android.graphics.Matrix;
import android.widget.EditText;
import java.security.SecureRandom;
import android.content.res.Configuration;
import android.util.DisplayMetrics;

public class JapaneseCustomRomSettings extends PreferenceFragment
        implements FileListDialog.onFileListDialogListener {

    private static final String TABLET_UI_PROPERTY = "persist.sys.force.tablet";
    private static final String ACTIONBAR_BOTTOM_PROPERTY = "persist.sys.actionbar.bottom";
    private static final String MY_FONT_PROPERTY = "persist.sys.force.myfont";
    private static final String MY_HOBBY_PROPERTY = "persist.sys.force.hobby";
    private static final String MY_THEME_PROPERTY = "persist.sys.theme";
    private static final String MY_SEFFECTS_PROPERTY = "persist.sys.sound.effects";
    private static final String MY_WALLPAPER_PROPERTY = "persist.sys.fixed.wallpaper";
    private static final String MY_HOMESCREEN_PROPERTY = "persist.sys.num.homescreen";

    private static final String FORCE_TABLET_UI_KEY = "force_tablet_ui";
    private static final String ACTIONBAR_BOTTOM_KEY = "actionbar_bottom";
    private static final String FORCE_MY_FONT_KEY = "force_my_font";
    private static final String FORCE_MY_HOBBY_KEY = "force_my_hobby";
    private static final String THEME_KEY = "theme_setting";
    private static final String DEVINFO_KEY = "jcrom_developer";
    private static final String FORCE_FIXED_WALLPAPER = "force_fixed_wallpaper";
    private static final String NUM_OF_HOMESCREEN = "number_of_homescreen";
    private static final String FORCE_MY_ANDROID_ID_KEY = "force_my_android_id";

    private static final String TAG = "JapaneseCustomRomSettings";

    private CheckBoxPreference mForceTabletUi;
    private CheckBoxPreference mActionBarBottom;
    private CheckBoxPreference mForceMyFont;
    private CheckBoxPreference mForceMyHobby;
    private PreferenceScreen mTheme;
    private CheckBoxPreference mFixedWallpaper;
    private ListPreference mNumHomescreen;
    private PreferenceScreen mForceMyAndroidId;
    
    private String mAndroidId;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        addPreferencesFromResource(R.xml.jcrom_settings);

        mForceTabletUi = (CheckBoxPreference) findPreference(FORCE_TABLET_UI_KEY);
        mActionBarBottom = (CheckBoxPreference) findPreference(ACTIONBAR_BOTTOM_KEY);
        mForceMyFont = (CheckBoxPreference) findPreference(FORCE_MY_FONT_KEY);
        mForceMyHobby = (CheckBoxPreference) findPreference(FORCE_MY_HOBBY_KEY);
        mTheme = (PreferenceScreen) findPreference(THEME_KEY);
        mFixedWallpaper = (CheckBoxPreference) findPreference(FORCE_FIXED_WALLPAPER);
	 mNumHomescreen = (ListPreference) findPreference(NUM_OF_HOMESCREEN);
        mForceMyAndroidId = (PreferenceScreen) findPreference(FORCE_MY_ANDROID_ID_KEY);

        if ((SystemProperties.get(MY_THEME_PROPERTY) != null) && (SystemProperties.get(MY_THEME_PROPERTY) != "")) {
            mTheme.setSummary(SystemProperties.get(MY_THEME_PROPERTY));
        }
	 
       // No way to setsummary after changing number of homescreen ? undertesting.
        if ((SystemProperties.get(MY_HOMESCREEN_PROPERTY) != null) && (SystemProperties.get(MY_HOMESCREEN_PROPERTY) != "")) {
	    mNumHomescreen.setSummary(SystemProperties.get(MY_HOMESCREEN_PROPERTY));
	 }else{
           mNumHomescreen.setSummary(R.string.number_of_homescreen_summary);
        }

        mNumHomescreen.setOnPreferenceChangeListener(
              new OnPreferenceChangeListener() {
                    @Override
                    public boolean onPreferenceChange(Preference preference, Object newValue){
                          // TODO hand-generated method stub;;
                          ListPreference _list = (ListPreference)findPreference(NUM_OF_HOMESCREEN);
          
                          if(_list == preference && newValue != null){
                                 String screenNum = (String)newValue.toString();
                                      
                                 //Log.e(TAG, "ScreenNum" + screenNum); 
                                 _list.setSummary(screenNum);
                                 writeNumberofScreenOptions(screenNum);

                                 try {
                                      ActivityManager am = (ActivityManager)getActivity().getSystemService(Context.ACTIVITY_SERVICE);
                                      am.forceStopPackage("com.android.launcher");
                                 } catch (Exception e) {
                                      e.printStackTrace();
                                     }
                             }
                         return true;
                        }
                 });

        mAndroidId = Settings.Secure.getString(
        		getActivity().getContentResolver(), Settings.Secure.ANDROID_ID);
        mForceMyAndroidId.setSummary(mAndroidId);
        
    }

    private void updateTabletUiOptions() {
        mForceTabletUi.setChecked(SystemProperties.getBoolean(TABLET_UI_PROPERTY, false));
    }
    
    private void writeTabletUiOptions() {
        SystemProperties.set(TABLET_UI_PROPERTY, mForceTabletUi.isChecked() ? "true" : "false");
    }

    private void writeActionBarBottomOptions() {
        SystemProperties.set(ACTIONBAR_BOTTOM_PROPERTY, mActionBarBottom.isChecked() ? "true" : "false");
    }

    private void updateMyFontOptions() {
        mForceMyFont.setChecked(SystemProperties.getBoolean(MY_FONT_PROPERTY, false));
    }
    
    private void writeMyFontOptions() {
        SystemProperties.set(MY_FONT_PROPERTY, mForceMyFont.isChecked() ? "true" : "false");
    }

    private void updateMyHobbyOptions() {
        mForceMyFont.setChecked(SystemProperties.getBoolean(MY_HOBBY_PROPERTY, false));
    }
    
    private void writeMyHobbyOptions() {
        SystemProperties.set(MY_HOBBY_PROPERTY, mForceMyHobby.isChecked() ? "true" : "false");
        if (!(mForceMyHobby.isChecked())) {
            SystemProperties.set(MY_THEME_PROPERTY, "");
            mTheme.setSummary("");
            setDefaultSounds();
            themeAllClear();
            try {
                ActivityManager am = (ActivityManager)getActivity().getSystemService(Context.ACTIVITY_SERVICE);
                am.forceStopPackage("com.android.launcher");
                Runtime.getRuntime().exec("pkill -f  com.android.systemui");
                Thread.sleep(7500);
            } catch (Exception e) {
                e.printStackTrace();
            }
            try {
                WallpaperManager.getInstance(getActivity()).clear();
            } catch (IOException e) {
            }
        }
    }
 
    private void updateMyWallpaperOptions() {
        mFixedWallpaper.setChecked(SystemProperties.getBoolean(MY_WALLPAPER_PROPERTY, false));
    }

    private void writeMyWallpaperOptions() {
        SystemProperties.set(MY_WALLPAPER_PROPERTY, mFixedWallpaper.isChecked() ? "true" : "false");
    }

    private void writeNumberofScreenOptions(String screenNum) {
        SystemProperties.set(MY_HOMESCREEN_PROPERTY, screenNum); 
    }
    
    private void writeForceMyAndroidId(String newAndroidId) {
    	if(newAndroidId == null || newAndroidId.equals("")) {
    		final SecureRandom random = new SecureRandom();
    		newAndroidId = Long.toHexString(random.nextLong());
    	}
	    Settings.Secure.putString(getActivity().getContentResolver()
	    		, Settings.Secure.ANDROID_ID, newAndroidId);
	    mForceMyAndroidId.setSummary(newAndroidId);
	    mAndroidId = newAndroidId;
    }

    private void setDefaultSounds() {
        Settings.System.putString(getActivity().getContentResolver(), Settings.System.LOW_BATTERY_SOUND, 
                                    "/system/media/audio/ui/LowBattery.ogg");
        Settings.System.putString(getActivity().getContentResolver(), Settings.System.DESK_DOCK_SOUND, 
                                    "/system/media/audio/ui/Dock.ogg");
        Settings.System.putString(getActivity().getContentResolver(), Settings.System.DESK_UNDOCK_SOUND, 
                                    "/system/media/audio/ui/Undock.ogg");
        Settings.System.putString(getActivity().getContentResolver(), Settings.System.CAR_DOCK_SOUND, 
                                    "/system/media/audio/ui/Dock.ogg");
        Settings.System.putString(getActivity().getContentResolver(), Settings.System.CAR_UNDOCK_SOUND, 
                                    "/system/media/audio/ui/Undock.ogg");
        Settings.System.putString(getActivity().getContentResolver(), Settings.System.LOCK_SOUND, 
                                    "/system/media/audio/ui/Lock.ogg");
        Settings.System.putString(getActivity().getContentResolver(), Settings.System.UNLOCK_SOUND, 
                                    "/system/media/audio/ui/Unlock.ogg");
    }

    private void setDataBase(String key, String name) {
        StringBuilder builder = new StringBuilder();
        //builder.append(Environment.getExternalStorageDirectory().toString() + "/mytheme/" + SystemProperties.get("persist.sys.theme") + "/sounds/effect/");
        builder.append(Environment.getDataDirectory().toString() + "/theme/sounds/effect/");
        builder.append(File.separator);
        builder.append(name);
        String filePath = builder.toString();
        File file = new File(filePath);
        if (file.exists()) {
            Settings.System.putString(getActivity().getContentResolver(), key, filePath);
        }
    }

    private void setMySounds() {
        String forceHobby = SystemProperties.get("persist.sys.force.hobby");
        if (forceHobby.equals("true")) {
            setDataBase(Settings.System.LOW_BATTERY_SOUND, "LowBattery.ogg");
            setDataBase(Settings.System.DESK_DOCK_SOUND, "Dock.ogg");
            setDataBase(Settings.System.DESK_UNDOCK_SOUND, "UnDock.ogg");
            setDataBase(Settings.System.CAR_DOCK_SOUND, "CarDock.ogg");
            setDataBase(Settings.System.CAR_UNDOCK_SOUND, "UnCarDock.ogg");
            setDataBase(Settings.System.LOCK_SOUND, "Lock.ogg");
            setDataBase(Settings.System.UNLOCK_SOUND, "unLock.ogg");
        }
    }

    public void themeCopy(File iDir, File oDir) {
        if (iDir.isDirectory()) {
            String[] children = iDir.list();
            for (int i=0; i<children.length; i++) {
                File iFile = new File(iDir, children[i]);
                File oFile = new File(oDir, children[i]);

                try {
                    FileChannel iChannel = new FileInputStream(iFile).getChannel();
                    FileChannel oChannel = new FileOutputStream(oFile).getChannel();
                    iChannel.transferTo(0, iChannel.size(), oChannel);
                    iChannel.close();
                    oChannel.close();
                    oFile.setReadable(true, false);
                } catch (FileNotFoundException e) {
                    e.printStackTrace();
                } catch (IOException e) {
                    e.printStackTrace();
                }

            }
        }
    }

    public void themeInstall(String parts) {
        StringBuilder ibuilder = new StringBuilder();
        StringBuilder obuilder = new StringBuilder();
        ibuilder.append(Environment.getExternalStorageDirectory().toString() + "/mytheme/" + SystemProperties.get("persist.sys.theme") + "/" + parts + "/");
        obuilder.append(Environment.getDataDirectory().toString() + "/theme/" + parts + "/");
        String iDirPath = ibuilder.toString();
        String oDirPath = obuilder.toString();
        File iDir = new File(iDirPath);
        File oDir = new File(oDirPath);
        themeCopy(iDir, oDir);
    }

    public void themeDelete(File iDir) {
        if (iDir.isDirectory()) {
            String[] children = iDir.list();
            for (int i=0; i<children.length; i++) {
                File iFile = new File(iDir, children[i]);
                iFile.delete();
            }
        }
    }

    public void themeClear(String parts) {
        StringBuilder ibuilder = new StringBuilder();
        ibuilder.append(Environment.getDataDirectory().toString() + "/theme/" + parts + "/");
        String iDirPath = ibuilder.toString();
        File iDir = new File(iDirPath);
        themeDelete(iDir);
    }

    public void themeAllClear() {
        themeClear("bootanime");
        themeClear("frame");
        themeClear("launcher");
        themeClear("lockscreen");
        themeClear("navikey");
        themeClear("notification");
        themeClear("screenshot");
        themeClear("statusbar");
        themeClear("navibar");
        themeClear("simeji");
        themeClear("sounds/effect");
        themeClear("sounds/bootsound");
        themeClear("wallpaper");
    }

    @Override
    public void onClickFileList(File file) { 
        if(file != null) {
            SystemProperties.set(MY_THEME_PROPERTY, file.getName());
            mTheme.setSummary(file.getName());

            themeAllClear();
            themeInstall("bootanime");
            themeInstall("frame");
            themeInstall("launcher");
            themeInstall("lockscreen");
            themeInstall("navikey");
            themeInstall("notification");
            themeInstall("screenshot");
            themeInstall("statusbar");
            themeInstall("navibar");
            themeInstall("simeji");
            themeInstall("sounds/effect");
            themeInstall("sounds/bootsound");
            themeInstall("wallpaper");

            setDefaultSounds();
            setMySounds();

            try {
                ActivityManager am = (ActivityManager)getActivity().getSystemService(Context.ACTIVITY_SERVICE);
                am.forceStopPackage("com.android.launcher");
                Runtime.getRuntime().exec("pkill -f  com.android.systemui");
                Thread.sleep(7500);
            } catch (Exception e) {
                e.printStackTrace();
            }

            Bitmap bitmapWallpaper;
            String MY_FRAME_FILE = "home_wallpaper.png";
            StringBuilder builder = new StringBuilder();
            //builder.append(Environment.getExternalStorageDirectory().toString() + "/mytheme/" + SystemProperties.get("persist.sys.theme") + "/wallpaper/");
            builder.append(Environment.getDataDirectory().toString() + "/theme/wallpaper/");
            builder.append(File.separator);
            builder.append(MY_FRAME_FILE);
            String filePath = builder.toString();
            bitmapWallpaper = BitmapFactory.decodeFile(filePath);
            if (null != bitmapWallpaper) {
                try {
                    WallpaperManager wm = WallpaperManager.getInstance(getActivity());
                    int srcWidth = bitmapWallpaper.getWidth();
                    int srcHeight = bitmapWallpaper.getHeight();

                    int screenSize = getResources().getConfiguration().screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK;
                    boolean isScreenLarge = screenSize == Configuration.SCREENLAYOUT_SIZE_LARGE || screenSize == Configuration.SCREENLAYOUT_SIZE_XLARGE;
                    DisplayMetrics displayMetrics = new DisplayMetrics();
                    getActivity().getWindowManager().getDefaultDisplay().getRealMetrics(displayMetrics);
                    int maxDim = Math.max(displayMetrics.widthPixels, displayMetrics.heightPixels);
                    int minDim = Math.min(displayMetrics.widthPixels, displayMetrics.heightPixels);
                    float WALLPAPER_SCREENS_SPAN = 2f;
                    int w, h;
                    if (isScreenLarge) {
                        w = (int) (maxDim * wallpaperTravelToScreenWidthRatio(maxDim, minDim));
                        h = maxDim;
                    } else {
                        w = Math.max((int) (minDim * WALLPAPER_SCREENS_SPAN), maxDim);
                        h = maxDim;
                    }

                    if(w < srcWidth && h < srcHeight){
                        Matrix matrix = new Matrix();
                        float widthScale = w / (float)srcWidth;
                        float heightScale = h / (float)srcHeight;
                        matrix.postScale(widthScale, heightScale);
                        Bitmap resizedWallpaper = Bitmap.createBitmap(bitmapWallpaper, 0, 0, srcWidth, srcHeight, matrix, true);
                       wm.setBitmap(resizedWallpaper);
                    }else{
                       wm.setBitmap(bitmapWallpaper);
                    }
                } catch (IOException e) {
                }
            }
        }
    }

    // borrowed from "com/android/launcher2/Workspace.java"
    private float wallpaperTravelToScreenWidthRatio(int width, int height) {

        float aspectRatio = width / (float) height;

        final float ASPECT_RATIO_LANDSCAPE = 16/10f;
        final float ASPECT_RATIO_PORTRAIT = 10/16f;
        final float WALLPAPER_WIDTH_TO_SCREEN_RATIO_LANDSCAPE = 1.5f;
        final float WALLPAPER_WIDTH_TO_SCREEN_RATIO_PORTRAIT = 1.2f;

        final float x =
            (WALLPAPER_WIDTH_TO_SCREEN_RATIO_LANDSCAPE - WALLPAPER_WIDTH_TO_SCREEN_RATIO_PORTRAIT) /
            (ASPECT_RATIO_LANDSCAPE - ASPECT_RATIO_PORTRAIT);
        final float y = WALLPAPER_WIDTH_TO_SCREEN_RATIO_PORTRAIT - x * ASPECT_RATIO_PORTRAIT;
        return x * aspectRatio + y;
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {

        if (Utils.isMonkeyRunning()) {
            return false;
        }

        if (preference == mForceTabletUi) {
            writeTabletUiOptions();
        } else if (preference == mActionBarBottom) {
            writeActionBarBottomOptions();
        } else if (preference == mForceMyFont) {
            writeMyFontOptions();
        } else if (preference == mForceMyHobby) {
            writeMyHobbyOptions();
        } else if (preference == mTheme) {
            if(mForceMyHobby.isChecked()) {
                FileListDialog dlg = new FileListDialog(getActivity());
                dlg.setOnFileListDialogListener(this);
                dlg.show( "/sdcard/mytheme/", "select theme");
            }
        } else if (preference == mFixedWallpaper) {
            writeMyWallpaperOptions();
        } else if (preference == mForceMyAndroidId) {
        	showNewAndroidIdDialog();
        }

        return false;
    }
    
    private void showNewAndroidIdDialog() {
    	final EditText editView = new EditText(getActivity());
    	editView.setText(mAndroidId);
    	AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
    	builder.setTitle(R.string.force_my_android_id);
    	builder.setView(editView);
    	builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
    		public void onClick(DialogInterface dialog, int whichButton) {
    			String newAndroidId = editView.getText().toString();
    			writeForceMyAndroidId(newAndroidId);
    		}
    	});
    	builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
    		public void onClick(DialogInterface dialog, int whichButton) {
    		}
    	});
    	builder.show();
    }

}

