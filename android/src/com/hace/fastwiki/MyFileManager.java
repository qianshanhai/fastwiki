package com.hace.fastwiki;  

import java.io.File;  
import java.io.IOException;
import java.util.ArrayList;  
import java.util.List;  
import java.util.Arrays;
import java.util.Comparator;

import android.app.Activity;
import android.app.ListActivity;  
import android.content.Intent;  
import android.net.Uri;  
import android.os.Bundle;  
import android.util.Log;  
import android.view.View;  
import android.view.View.OnClickListener;  
import android.view.Window;  
import android.widget.Button;  
import android.widget.ListView;  
import android.widget.TextView;  

public class MyFileManager extends ListActivity {  
	private List<String> items = null;  
	private List<String> paths = null;  
	private String rootPath = "/";  
	private String curPath = "/";  
	private TextView mPath;  
	private final static String TAG = "bb";  
	@Override  
		protected void onCreate(Bundle icicle) {  
			super.onCreate(icicle);  
			requestWindowFeature(Window.FEATURE_NO_TITLE);  
			setContentView(R.layout.fileselect);  
			mPath = (TextView) findViewById(R.id.mPath);  
			Button buttonConfirm = (Button) findViewById(R.id.buttonConfirm);  
			buttonConfirm.setText(N("SELECT_FOLDER_OK"));

			buttonConfirm.setOnClickListener(new OnClickListener() {  
				public void onClick(View v) {  
					Intent data = new Intent(MyFileManager.this, ListActivity.class);  
					Bundle bundle = new Bundle();  
					bundle.putString("file", curPath);  
					data.putExtras(bundle);  
					setResult(2, data);  
					finish();  
				}  
			});  
			Button buttonCancle = (Button) findViewById(R.id.buttonCancle);  
			buttonCancle.setText(N("SELECT_FOLDER_CANCLE"));
			buttonCancle.setOnClickListener(new OnClickListener() {  
				public void onClick(View v) {  
					finish();  
				}  
			});  
			getFileDir(rootPath);  
		}  

	private void getFileDir(String filePath) {  
		mPath.setText(filePath);  
		items = new ArrayList<String>();  
		paths = new ArrayList<String>();  
		File f = new File(filePath);  
		File[] files = f.listFiles();  

		Arrays.sort( files, new Comparator() {
				public int compare(final Object o1, final Object o2) {
					String a = ((File)o1).getName() ;
					String b = ((File)o2).getName() ;
					return a.compareTo(b);
				}
			 }
		); 


		if (!filePath.equals(rootPath)) {  
			items.add("b1");  
			paths.add(rootPath);  
			items.add("b2");  
			paths.add(f.getParent());  
		} 
		if (files != null) {
			for (int i = 0; i < files.length; i++) {  
				File file = files[i];  
				String name = file.getName();
				if (file.canRead() && !name.substring(0,1).equals(".")) {
					items.add(file.getName()); 
					paths.add(file.getPath());  
				}
			}  
		}
		setListAdapter(new MyAdapter(this, items, paths));  
	}  
	@Override  

	protected void onListItemClick(ListView l, View v, int position, long id) {  
		File file = new File(paths.get(position));  
		if (file.isDirectory()) {  
			curPath = paths.get(position);  
			getFileDir(paths.get(position));  
		} else {  
			// open file
		}  
	}  
	public native String N(String name);
}
