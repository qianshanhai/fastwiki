/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
package com.hace.fastwiki;

import android.app.Activity;  
import android.app.AlertDialog;  
import android.content.DialogInterface;  
import android.os.Bundle;  

import android.view.View;
import android.view.Window;
import android.view.LayoutInflater;
import android.widget.ImageButton;

import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import android.widget.CheckBox;  
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.view.WindowManager;

import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.text.Html;
import android.text.Spanned;

import android.content.Context;
import android.content.Intent;  
import android.graphics.Color; 

import com.actionbarsherlock.app.SherlockActivity;
import com.actionbarsherlock.app.ActionBar;
import com.actionbarsherlock.view.Menu;
import com.actionbarsherlock.view.SubMenu;
import com.actionbarsherlock.view.MenuItem;

import android.widget.ImageView;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.view.KeyEvent;

public class FileBrowse extends SherlockActivity {
	private TextView m_text = null;
	private ListView m_list_view = null;
	private String [] m_files = null;
	private FileBrowseAdapter m_adapter = null;
	private ViewGroup customViewForDictView;
	private TextView m_last_text = null;
	private int m_idx = -1;
	String m_curr_path = "/mnt/sdcard";

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState); 

		setContentView(R.layout.file_browse);  

		final ActionBar ab = getSupportActionBar();
		ab.setDisplayHomeAsUpEnabled(false);
		ab.setDisplayShowCustomEnabled(true);
		ab.setDisplayShowTitleEnabled(false);
		ab.setHomeButtonEnabled(true);

		customViewForDictView = (ViewGroup) getLayoutInflater().inflate(R.layout.file_browse_bar, null);
		ab.setCustomView(customViewForDictView);

		m_text = (TextView)customViewForDictView.findViewById(R.id.fb_text);
		m_text.setText(N("FW_FB_BODY_IMAGE"));

		m_list_view = (ListView)findViewById(R.id.file_browse_list);

		m_files = GetFiles(m_curr_path);
		view_file_browse();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getSupportMenuInflater().inflate(R.menu.file_browse, menu);

		menu.findItem(R.id.fb_menu_ok).setTitle(N("FW_FB_OK"));

		return super.onCreateOptionsMenu(menu);
	}

	void return_val(String m)
	{
		Intent data = new Intent(FileBrowse.this, Activity.class);  
		Bundle bundle = new Bundle();  
		bundle.putString("body_path", m);
		data.putExtras(bundle); 
		setResult(200, data);  
		finish();
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
			case R.id.fb_menu_ok:
				if (m_last_text == null) {
					AlertDialog.Builder alert = new AlertDialog.Builder(this);
					alert.setMessage(N("FW_FB_SELECT_ONE"));
					alert.setPositiveButton(N("FW_FB_OK"),
							new DialogInterface.OnClickListener(){  
								public void onClick(DialogInterface dialoginterface, int i) {
								}
							});
					alert.show();
				} else {
					return_val(m_curr_path + "/" + m_last_text.getText().toString());
				}
				break;
			default:
				break;
		}
		return true;
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			if (m_curr_path.equals("/") || m_curr_path.equals("/mnt/sdcard"))
				return_val("");
			else {
				m_curr_path = RealPath(m_curr_path + "/..");
				m_files = GetFiles(m_curr_path);
				m_adapter.notifyDataSetChanged();
			}
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}

	private void view_file_browse()
	{
		m_adapter = new FileBrowseAdapter(this);

		m_list_view.setAdapter(m_adapter);
		m_list_view.setOnItemClickListener(new OnItemClickListener() {
			public void onItemClick(AdapterView<?> arg0, View v, int pos, long id) {
				ViewHolder holder = (ViewHolder) v.getTag();
				String m = holder.text.getText().toString();
				if (holder.flag == 0) {
					if (m_idx >= 0 && m_last_text != null) {
						m_last_text.setBackgroundColor(Color.parseColor("#F8F8FF"));
					}
					m_idx = pos;
					holder.text.setBackgroundColor(Color.parseColor("#1E90FF"));
					m_last_text = holder.text;
				} else {
					m_idx = -1;
					m_curr_path = RealPath(m_curr_path + "/" + m);
					m_files = GetFiles(m_curr_path);
					m_adapter.notifyDataSetChanged();
				}
			}
		});
	}

	public class FileBrowseAdapter extends BaseAdapter
	{
		private LayoutInflater mInflater;
		private Bitmap m_folder;
		private Bitmap m_doc;

		public FileBrowseAdapter(Context context)
		{
			mInflater = LayoutInflater.from(context);

			m_folder = BitmapFactory.decodeResource(context.getResources(),R.drawable.file_folder);
			m_doc = BitmapFactory.decodeResource(context.getResources(),R.drawable.file_doc);
		}

		@Override
		public int getCount() { return m_files.length; } 

		@Override
		public Object getItem(int position) { return m_files[position]; }

		@Override
		public long getItemId(int position) { return position; }

		@Override
		public View getView(int position, View convertView, ViewGroup parent)
		{
			ViewHolder holder;

			if (convertView == null) {
				convertView = mInflater.inflate(R.layout.file_browse_one, null);
				holder = new ViewHolder();
				holder.img = (ImageView)convertView.findViewById(R.id.file_browse_img);
				holder.text = (TextView)convertView.findViewById(R.id.file_browse_text);
				convertView.setTag(holder);
			} else {
				holder = (ViewHolder) convertView.getTag();
			}

			String m = m_files[position];

			holder.text.setText(m.substring(1));
			holder.text.setBackgroundColor(Color.parseColor("#F8F8FF"));

			if (m.substring(0, 1).equals("0")) {
				holder.flag = 0;
				holder.img.setImageBitmap(m_doc);
				if (m_idx == position) {
					holder.text.setBackgroundColor(Color.parseColor("#1E90FF"));
				}
			} else {
				holder.flag = 1;
				holder.img.setImageBitmap(m_folder);
			}

			return convertView;
		}

	}
	private class ViewHolder
	{
		ImageView img;
		TextView text;
		int flag;
	}

	public native String [] GetFiles(String path);
	public native String N(String name);
	public native String RealPath(String path);
}
