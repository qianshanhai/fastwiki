/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
package com.hace.fastwiki;

import android.view.View;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.Window;
import android.view.KeyEvent;
import android.view.Display;
import android.view.WindowManager;
import android.view.MotionEvent;
import android.view.inputmethod.InputMethodManager;
import android.view.View.OnKeyListener;
import android.view.LayoutInflater;
import android.view.Gravity;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import android.text.Html;
import android.graphics.Color; 

import android.widget.TextView;
import android.widget.RelativeLayout;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.Toast;
import android.widget.ScrollView;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;

import android.text.TextWatcher;
import android.text.Editable;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.preference.*;

import android.widget.ImageButton;
import android.widget.EditText;
import android.app.ListActivity;


import com.actionbarsherlock.app.SherlockActivity;
import com.actionbarsherlock.app.ActionBar;

public class FastwikiFind extends SherlockActivity {
	private ViewGroup customViewForDictView;
	private EditText myUrl;  
	private ImageButton Delete;
	private int change_flag = 0; 
	private ListView m_list_view = null;
	private String [] m_list_color = null;
	private int not_found = 0;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState); 

		setContentView(R.layout.match_main);  

		m_list_color = WikiListColor();

		Intent intent = getIntent();
		String title = intent.getStringExtra("key");

		final ActionBar ab = getSupportActionBar();
		ab.setDisplayHomeAsUpEnabled(false);
		ab.setDisplayShowCustomEnabled(true);
		ab.setDisplayShowTitleEnabled(false);
		ab.setHomeButtonEnabled(true);

		customViewForDictView = (ViewGroup) getLayoutInflater().inflate(R.layout.search_edit, null);
		ab.setCustomView(customViewForDictView);

		Delete = (ImageButton)customViewForDictView.findViewById(R.id.delete01);

		m_list_view = (ListView)findViewById(R.id.listview);

		myUrl = (EditText)customViewForDictView.findViewById(R.id.url);  
		myUrl.setText(title);
		myUrl.setSelection(title.length());

		on_change();

		myUrl.addTextChangedListener(new TextWatcher(){
			public void afterTextChanged(Editable s) {
			}
			public void beforeTextChanged(CharSequence s, int start, int count, int after){
			}
			public void onTextChanged(CharSequence s, int start, int before, int count){
				if (change_flag == 0)
					on_change();
			}

		});
	}

	public void on_change()
	{
		MatchAdapter adapter;
		String [][] items;
		String m = myUrl.getText().toString();

		if (m.equals(""))
			return;

		String [] x = WikiMatch(m);
		String [] y = WikiMatchLang();

		if (x.length == 0) {
			items = new String [1][2];
			not_found = 1;
			items[0][0] = "Not found";
			items[0][1] = "";
		} else {
			not_found = 0;
			items = new String [x.length][2];

			for (int i = 0; i < x.length; i++) {
				items[i][0] = x[i];
				items[i][1] = y[i];
			}
		}

		adapter = new MatchAdapter(this, items);

		m_list_view.setAdapter(adapter);

		m_list_view.setOnItemClickListener(new OnItemClickListener() {
			public void onItemClick(AdapterView<?> arg0, View v, int pos, long id) {
				if (not_found == 0) {
					Intent data = new Intent(FastwikiFind.this, ListActivity.class);
					Bundle bundle = new Bundle();
					bundle.putString("pos", pos + "");
					data.putExtras(bundle);
					setResult(2, data);
					finish();
				}
			}
		});

		return;
	}

	 private class MatchAdapter extends BaseAdapter
	{
		private LayoutInflater mInflater;
		private String [][] m_item;

		public MatchAdapter(Context context, String [][] it)
		{
			mInflater = LayoutInflater.from(context);

			m_item = it;
		}

		@Override
		public int getCount()
		{ 
			return m_item.length;
		} 

		@Override
		public Object getItem(int position)
		{
			return m_item[position];
		}

		@Override
		public long getItemId(int position)
		{
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent)
		{
			ViewHolder holder;

			if (convertView == null) {
				convertView = mInflater.inflate(R.layout.match_one, null);
				holder = new ViewHolder();
				holder.title  = (TextView)convertView.findViewById(R.id.match_title);
				holder.lang = (TextView)convertView.findViewById(R.id.match_lang);
				convertView.setTag(holder);
			} else {
				holder = (ViewHolder) convertView.getTag();
			}

			holder.title.setText(Html.fromHtml(m_item[position][0].toString()));
			holder.title.setBackgroundColor(Color.parseColor(m_list_color[0]));
			holder.title.setTextColor(Color.parseColor(m_list_color[1]));

			if (m_item[position][1].equals("")) {
				holder.lang.setVisibility(View.GONE);
			} else {
				holder.lang.setVisibility(View.VISIBLE);
				holder.lang.setText(Html.fromHtml(m_item[position][1].toString()));
				holder.lang.setBackgroundColor(Color.parseColor(m_list_color[0]));
				holder.lang.setTextColor(Color.parseColor(m_list_color[1]));
			}

			return convertView;
		}
		private class ViewHolder
		{
			TextView title;
			TextView lang;
		}
	}
	public native String [] WikiMatch(String key);
	public native String [] WikiMatchLang();
	public native String [] WikiListColor();
}
