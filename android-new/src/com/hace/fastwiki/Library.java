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

public class Library extends SherlockActivity {
	private ListView m_list_view = null;
	private int [] m_idx;
	private int [] m_backup;
	private TextView m_text = null;
	private String [] m_lang = null;
	private String [] m_list_color = null;
	private LibraryAdapter m_adapter = null;
	private boolean m_select_flag = false;
	private ViewGroup customViewForDictView;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState); 

		setContentView(R.layout.library);  

		final ActionBar ab = getSupportActionBar();
		ab.setDisplayHomeAsUpEnabled(false);
		ab.setDisplayShowCustomEnabled(true);
		ab.setDisplayShowTitleEnabled(false);
		ab.setHomeButtonEnabled(true);

		customViewForDictView = (ViewGroup) getLayoutInflater().inflate(R.layout.library_bar, null);
		ab.setCustomView(customViewForDictView);

		m_text = (TextView)customViewForDictView.findViewById(R.id.library_text);
		m_text.setText(N("FW_LIBRARY_LIST"));

		m_list_view = (ListView)findViewById(R.id.library_lv);  
		m_list_color = WikiListColor();

		m_lang  = WikiLangList();

		m_idx = new int[128];
		m_backup = new int[128];

		init_idx_backup();

		view_library();
		init_full_screen();
	}

	private void init_idx_backup()
	{
		String [] curr_lang = WikiCurrLang();

		for (int i = 0; i < m_lang.length; i++) {
			int found = 0;
			for (int j = 0; j < curr_lang.length; j++) { 
				if (m_lang[i].equals(curr_lang[j])) {
					found = 1;
					break;
				}
			}
			m_idx[i] = found;
			m_backup[i] = found;
		}
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getSupportMenuInflater().inflate(R.menu.library_menu, menu);

		menu.findItem(R.id.library_menu_ok).setTitle(N("FW_LIBRARY_OK"));
		menu.findItem(R.id.library_menu_all).setTitle(N("FW_LIBRARY_SELECT_ALL"));

		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
			case R.id.library_menu_ok:
				if (select_total() > 0) {
					SetSelectLang(m_idx);
					finish();
				} else {
					show_short_msg(N("FW_LIBRARY_LESS_1"));
				}
				return true;
			case R.id.library_menu_all:
				if (m_select_flag) {
					m_select_flag = false;
					select_backup();
				} else {
					m_select_flag = true;
					select_all();
				}
				break;
			default:
				return super.onOptionsItemSelected(item);
		}
		return true;
	}

	private int select_all()
	{
		for (int i = 0; i < m_idx.length; i++) {
			m_idx[i] = 1;
		}
		m_adapter.notifyDataSetChanged();

		return 0;
	}

	private int select_backup()
	{
		for (int i = 0; i < m_idx.length; i++) {
			m_idx[i] = m_backup[i];
		}
		m_adapter.notifyDataSetChanged();

		return 0;
	}

	private int select_total()
	{
		int total = 0;

		for (int i = 0; i < m_idx.length; i++) {
			if (m_idx[i] == 1)
				total++;
		}

		return total;
	}

	private void view_library()
	{
		m_adapter = new LibraryAdapter(this);

		m_list_view.setAdapter(m_adapter);
		m_list_view.setOnItemClickListener(new OnItemClickListener() {
			public void onItemClick(AdapterView<?> arg0, View v, int pos, long id) {
				ViewHolder holder = (ViewHolder) v.getTag();
				holder.box.toggle();
				if (m_idx[pos] == 1) {
					m_idx[pos] = 0;
			 		holder.box.setChecked(false);
				} else {
					m_idx[pos] = 1;
					holder.box.setChecked(true);
				}
			}
		});
	}

	public class LibraryAdapter extends BaseAdapter
	{
		private LayoutInflater mInflater;

		public LibraryAdapter(Context context)
		{
			mInflater = LayoutInflater.from(context);
		}

		@Override
		public int getCount() { return m_lang.length; } 

		@Override
		public Object getItem(int position) { return m_lang[position]; }

		@Override
		public long getItemId(int position) { return position; }

		@Override
		public View getView(int position, View convertView, ViewGroup parent)
		{
			ViewHolder holder;

			if (convertView == null) {
				convertView = mInflater.inflate(R.layout.library_one, null);
				holder = new ViewHolder();
				holder.text = (TextView)convertView.findViewById(R.id.library_text);
				holder.box = (CheckBox)convertView.findViewById(R.id.library_box);
				convertView.setTag(holder);
			} else {
				holder = (ViewHolder) convertView.getTag();
			}

			String m = m_lang[position];
			/*
			   Spanned x = Html.fromHtml(m);
			   LOG("history pos=" + position + ": " + x + "\n");
			   */

			holder.text.setText(m);
			holder.text.setBackgroundColor(Color.parseColor(m_list_color[0]));
			holder.text.setTextColor(Color.parseColor(m_list_color[1]));

			if (m_idx[position] == 1) {
				holder.box.setChecked(true);
			} else {
				holder.box.setChecked(false);
			}

			return convertView;
		}

	}
	private class ViewHolder
	{
		TextView text;
		CheckBox box;
	}

	private void show_short_msg(String msg)
	{
		Toast toast = Toast.makeText(Library.this, msg, Toast.LENGTH_SHORT); 
		toast.show();
	}

	private void init_full_screen()
	{
		if (WikiGetFullScreenFlag() == 1) {
			set_full_screen();
		}
	}

    private void set_full_screen()
	{
         this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				 	WindowManager.LayoutParams.FLAG_FULLSCREEN);
    }

	public native int SetSelectLang(int [] m);
	public native String[] WikiLangList();
	public native String[] WikiCurrLang();
	public native String [] WikiListColor();
	public native String N(String name);
	public native int WikiGetFullScreenFlag();
}
