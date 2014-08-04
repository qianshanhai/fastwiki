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

public class History extends SherlockActivity {
	private ListView m_list_view = null;
	private TextView m_text = null;
	private int m_select_total = 0;
	private boolean [] m_idx;
	private String [] m_history = null;
	private int switch_ui = 0;
	private String [] m_list_color = null;
	private ViewGroup customViewForDictView;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState); 

		setContentView(R.layout.history);  

		final ActionBar ab = getSupportActionBar();
		ab.setDisplayHomeAsUpEnabled(false);
		ab.setDisplayShowCustomEnabled(true);
		ab.setDisplayShowTitleEnabled(false);
		ab.setHomeButtonEnabled(true);

		customViewForDictView = (ViewGroup) getLayoutInflater().inflate(R.layout.history_bar, null);
		ab.setCustomView(customViewForDictView);

		m_list_color = WikiListColor();

		m_list_view = (ListView)findViewById(R.id.history_list);  
		m_text = (TextView)customViewForDictView.findViewById(R.id.history_txt);

		m_history = WikiHistory();
		m_idx = new boolean[m_history.length];

		for (int i = 0; i < m_history.length; i++) {
			m_idx[i] = false;
		}

		view_history();
		set_text();
		init_full_screen();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getSupportMenuInflater().inflate(R.menu.history_menu, menu);
		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
			case R.id.history_set:
				history_set();
				break;
			case R.id.history_delete:
				history_delete();
				break;
			default:
				return super.onOptionsItemSelected(item);
		}
		return true;
	}

	private void history_set()
	{
		if (switch_ui == 0) {
			switch_ui = 1;

			init_select_total();
			manage_history();
		} else {
			switch_ui = 0;
			init_select_total();
			view_history();
		}
		set_text();
	}

	private void history_delete()
	{
		String msg;

		if (m_select_total > 0) {
			msg = N("HISTORY_DELETE_1") + m_select_total + N("HISTORY_DELETE_2");
		} else {
			msg = N("HISTORY_CLEAN_MSG");
		}
		ask_delete(N("HISTORY_MANAGE"), msg, N("HISTORY_YES"), N("HISTORY_NO"));
	}

	private void init_select_total()
	{
		if (m_select_total == 0)
			return;

		m_select_total = 0;

		for (int i = 0; i < m_history.length; i++) {
			m_idx[i] = false;
		}
	}

	private void reload_history()
	{
		m_history = WikiHistory();
		init_select_total();
	}

	private void ask_delete(String title, String msg, String yes, String no)
	{
		AlertDialog.Builder alert = new AlertDialog.Builder(this);;
		alert.setTitle(title);
		alert.setMessage(msg);

		alert.setNegativeButton(no,
				new DialogInterface.OnClickListener(){  
					public void onClick(DialogInterface dialoginterface, int i) {
					}
				});

		alert.setPositiveButton(yes,
				new DialogInterface.OnClickListener(){  
					public void onClick(DialogInterface dialoginterface, int i) {
						if (m_select_total == 0) {
							clean_history();
						} else {
							delete_history();
						}
					}
		});
		alert.show();
	}

	private void delete_history()
	{
		for (int i = 0; i < m_history.length; i++) {
			if (m_idx[i]) {
				WikiHistoryDelete(i);
			}
		}
		reload_history();
		manage_history();
		set_text();
	}

	private void clean_history()
	{
		WikiHistoryClean();
		reload_history();
		view_history();
		set_text();
	}

	private void view_history()
	{
		HistoryAdapter adapter = new HistoryAdapter(this, m_history, 0);

		m_list_view.setAdapter(adapter);
		m_list_view.setOnItemClickListener(new OnItemClickListener() {
			public void onItemClick(AdapterView<?> arg0, View v, int pos, long id) {
				Intent data = new Intent(History.this, Activity.class);  
				Bundle bundle = new Bundle();  
				bundle.putInt("history_pos", pos);  
				bundle.putString("history_title", m_history[pos]);  
				data.putExtras(bundle); 
				setResult(2, data);  
				finish();
			}
		});
	}

	private void manage_history()
	{
		HistoryAdapter adapter = new HistoryAdapter(this, m_history, 1);

		m_list_view.setAdapter(adapter);
		m_list_view.setOnItemClickListener(new OnItemClickListener() {
			public void onItemClick(AdapterView<?> arg0, View v, int pos, long id) {
				ViewHolder holder = (ViewHolder) v.getTag();
				holder.box.toggle();

				if (m_idx[pos]) {
					m_select_total--;
				} else {
					m_select_total++;
				}
				m_idx[pos] = !m_idx[pos];
			 	holder.box.setChecked(m_idx[pos]);

				set_text();
			}
		});
	}

	private void set_text()
	{
		String m;

		if (m_select_total > 0) {
			m = N("HISTORY_TOTAL") + ": " + m_history.length
						+ "   " + N("HISTORY_SELECT") + ": " + m_select_total;
		} else {
			m = N("HISTORY_TOTAL") + ": " + m_history.length;
		}
		m_text.setText(String.format("%-22s   ", m));
	}

	 public class HistoryAdapter extends BaseAdapter
	 {
		 private LayoutInflater mInflater;
		 private String [] my_item;
		 int my_flag;

		 public HistoryAdapter(Context context, String [] it, int flag)
		 {
			 mInflater = LayoutInflater.from(context);
			 my_item = it;
			 my_flag = flag;
		 }

		 @Override
		 public int getCount() { return my_item.length; } 

		 @Override
		 public Object getItem(int position) { return my_item[position]; }

		 @Override
		 public long getItemId(int position) { return position; }

		 @Override
		 public View getView(int position, View convertView, ViewGroup parent)
		 {
			 ViewHolder holder;

			 if (convertView == null) {
				 convertView = mInflater.inflate(R.layout.history_one, null);
				 holder = new ViewHolder();
				 holder.text = (TextView)convertView.findViewById(R.id.history_text);
				 holder.box = (CheckBox)convertView.findViewById(R.id.history_box);
				 convertView.setTag(holder);
			 } else {
				 holder = (ViewHolder) convertView.getTag();
			 }

			 String m = my_item[position];
			 /*
			 Spanned x = Html.fromHtml(m);
			 LOG("history pos=" + position + ": " + x + "\n");
			 */

			 holder.text.setText(m);
			 holder.text.setBackgroundColor(Color.parseColor(m_list_color[0]));
			 holder.text.setTextColor(Color.parseColor(m_list_color[1]));

			 if (my_flag == 0) {
				 holder.box.setVisibility(View.GONE);
			 } else {
				 holder.box.setChecked(m_idx[position]);
				 holder.box.setVisibility(View.VISIBLE);
			 }

			 return convertView;
		 }

	 }

	 private class ViewHolder
	 {
		 TextView text;
		 CheckBox box;
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

	public native String N(String name);
	public native void LOG(String arg);

	public native String [] WikiHistory();
	public native int WikiGetFullScreenFlag();
	public native int WikiHistoryClean();
	public native int WikiHistoryDelete(int idx);
	public native String [] WikiListColor();
}
