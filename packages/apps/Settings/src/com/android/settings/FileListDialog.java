package com.android.settings;

import java.io.File;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.view.*;
import android.util.Log;
import java.util.Arrays;
import java.lang.Comparable;

public class FileListDialog extends Activity
    implements View.OnClickListener, DialogInterface.OnClickListener {

    private Context _parent = null;
    private File[] _dialog_file_list;
    private int _select_count = -1;
    private onFileListDialogListener _listener = null;
    private boolean _is_directory_select = false;
    private String zip = "zip";
    private String extension = null;
    private int[] theme_num;
    private static final String TAG = "FileListDialog";

    public void setDirectorySelect(boolean is){
        _is_directory_select = is;
    }
    public boolean isDirectorySelect(){
        return _is_directory_select;
    }
     
    public String getSelectedFileName(){
        String ret = "";
        if(_select_count < 0){
         
        }else{
            ret = _dialog_file_list[_select_count].getName();
        }
        return ret;
    }
     
    public FileListDialog(Context context){
        _parent = context;
    }
     
    @Override
    public void onClick(View v) {
    }
 
    @Override
    public void onClick(DialogInterface dialog, int which) {
        _select_count = theme_num[which];
        if((_dialog_file_list == null) || (_listener == null)){
        }else{
            File file = _dialog_file_list[theme_num[which]];
            _listener.onClickFileList(file);
        }
    }
 
    public String getSuffix(String fileName) {
        if (fileName == null)
            return null;
        int point = fileName.lastIndexOf(".");
        if (point != -1) {
            return fileName.substring(point + 1);
        }
        return fileName;
    }

    public void show(String path, String title){
         
        try{
            File[] _dialog_file_list_unsorted = new File(path).listFiles();

            FileWrapper [] fileWrappers = new FileWrapper[_dialog_file_list_unsorted.length];
            for (int i=0; i<_dialog_file_list_unsorted.length; i++) {
                fileWrappers[i] = new FileWrapper(_dialog_file_list_unsorted[i]);
            }
            Arrays.sort(fileWrappers);

            _dialog_file_list = new File[_dialog_file_list_unsorted.length];
            for (int i=0; i<_dialog_file_list_unsorted.length; i++) {
                _dialog_file_list[i] = fileWrappers[i].getFile();
            }

            theme_num = new int[_dialog_file_list.length];
            if(_dialog_file_list == null){
                //NG
                if(_listener != null){
                    _listener.onClickFileList(null);
                }
            }else{
                String[] list = new String[_dialog_file_list.length];
                int count = 0;
                int n = 0;
                String name = "";
 
                for (File file : _dialog_file_list) {
                    if(file.isDirectory()){
                        name = file.getName();
                    }else{
                        name = file.getName();
                    }
                    
                    extension = getSuffix(name);
                    if(!extension.equals(zip) && !".nomedia".equals(name)) {
                        list[count] = name;
                        theme_num[count] = n;
                        count++;
                    }
                    n++;
                }
                String[] theme_list = new String[count];
                for(int c=0; c < count; c++) {
                    theme_list[c] = list[c];
                }
                new AlertDialog.Builder(_parent).setTitle(title).setItems(theme_list, this).show();
            }
        }catch(SecurityException se){
        }catch(Exception e){
        }
         
    }
     
    public void setOnFileListDialogListener(onFileListDialogListener listener){
        _listener = listener;
    }
     
    public interface onFileListDialogListener{
        public void onClickFileList(File file);
    }
}

class FileWrapper implements Comparable{

    private File file;

    public FileWrapper(File file){
        this.file = file;
    }

    public int compareTo(Object obj){
        FileWrapper castObj = (FileWrapper)obj;

        if(this.file.getName().compareTo(castObj.getFile().getName()) > 0){
            return 1;    // large case
        }else if(this.file.getName().compareTo(castObj.getFile().getName()) < 0){
            return -1;    // small case
        }else{
            return 0;    // equal case
        }
    }

    public File getFile(){
        return this.file;
    }
}
