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

public class Favorite extends Activity {  
	private ListView m_list_view = null;
	private TextView m_text = null;
	private int m_select_total = 0;
	private boolean [] m_idx;
	private String []  m_favorite = null;
	private String []  m_favorite_rate = null;
	private ImageButton m_manage;
	private ImageButton m_delete;
	private int switch_ui = 0;
	private String [] m_list_color = null;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState); 

		this.requestWindowFeature(Window.FEATURE_NO_TITLE);  
		setContentView(R.layout.favorite);  

		m_list_color = WikiListColor();

		m_list_view = (ListView)findViewById(R.id.favorite_list);  
		m_text = (TextView)findViewById(R.id.favorite_txt);

		m_manage = (ImageButton)findViewById(R.id.favorite_manage);
		m_manage .setOnClickListener(new ImageButton.OnClickListener() {
			public void onClick(View v) {  
				if (switch_ui == 0) {
					switch_ui = 1;

					init_select_total();
					manage_favorite();
				} else {
					switch_ui = 0;
					init_select_total();
					view_favorite();
				}
				set_text();
			}
		});

		m_delete = (ImageButton)findViewById(R.id.favorite_delete);
		m_delete .setOnClickListener(new ImageButton.OnClickListener() {
			public void onClick(View v) { 
				String msg;

				if (m_select_total > 0) {
					msg = N("HISTORY_DELETE_1") + m_select_total + N("HISTORY_DELETE_2");
				} else {
					msg = N("HISTORY_CLEAN_MSG");
				}
				ask_delete(N("FAVORITE_MANAGE"), msg, N("HISTORY_YES"), N("HISTORY_NO"));
			}
		});

		m_favorite = WikiFavorite();
		m_favorite_rate = WikiFavoriteRate();
		m_idx = new boolean[m_favorite.length];

		for (int i = 0; i < m_favorite.length; i++) {
			m_idx[i] = false;
		}

		view_favorite();
		set_text();
		init_full_screen();
	}

	private void init_select_total()
	{
		if (m_select_total == 0)
			return;

		m_select_total = 0;

		for (int i = 0; i < m_favorite.length; i++) {
			m_idx[i] = false;
		}
	}

	private void reload_favorite()
	{
		m_favorite = WikiFavorite();
		m_favorite_rate = WikiFavoriteRate();
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
							clean_favorite();
						} else {
							delete_favorite();
						}
					}
		});
		alert.show();
	}

	private void delete_favorite()
	{
		for (int i = 0; i < m_favorite.length; i++) {
			if (m_idx[i]) {
				WikiFavoriteDelete(i);
			}
		}
		reload_favorite();
		manage_favorite();
		set_text();
	}

	private void clean_favorite()
	{
		WikiFavoriteClean();
		reload_favorite();
		view_favorite();
		set_text();
	}

	private void view_favorite()
	{
		m_delete.setVisibility(View.GONE);

		FavoriteAdapter adapter = new FavoriteAdapter(this, m_favorite, m_favorite_rate, 0);

		m_list_view.setAdapter(adapter);
		m_list_view.setOnItemClickListener(new OnItemClickListener() {
			public void onItemClick(AdapterView<?> arg0, View v, int pos, long id) {
				Intent data = new Intent(Favorite.this, Activity.class);  
				Bundle bundle = new Bundle();  
				bundle.putInt("favorite_pos", pos);  
				bundle.putString("favorite_title", m_favorite[pos]);  
				data.putExtras(bundle); 
				setResult(2, data);  
				finish();
			}
		});
	}

	private void manage_favorite()
	{
		m_delete.setVisibility(View.VISIBLE);
		FavoriteAdapter adapter = new FavoriteAdapter(this, m_favorite, m_favorite_rate, 1);

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
		if (m_select_total > 0) {
			m_text.setText(N("HISTORY_TOTAL") + ": " + m_favorite.length
						+ "   " + N("HISTORY_SELECT") + ": " + m_select_total);
		} else {
			m_text.setText(N("HISTORY_TOTAL") + ": " + m_favorite.length);
		}
	}

	 public class FavoriteAdapter extends BaseAdapter
	 {
		 private LayoutInflater mInflater;
		 private String [] my_item;
		 private String [] my_rate;
		 private int my_flag;

		 public FavoriteAdapter(Context context, String [] it, String [] rate, int flag)
		 {
			 mInflater = LayoutInflater.from(context);
			 my_item = it;
			 my_rate = rate;
			 my_flag = flag;
		 }

		 @Override
		 public int getCount()
		 { 
			 return my_item.length;
		 } 

		 @Override
		 public Object getItem(int position)
		 {
			 return my_item[position];
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
				 convertView = mInflater.inflate(R.layout.favorite_one, null);
				 holder = new ViewHolder();
				 holder.text = (TextView)convertView.findViewById(R.id.favorite_text);
				 holder.rate = (TextView)convertView.findViewById(R.id.favorite_rate);
				 holder.box = (CheckBox)convertView.findViewById(R.id.favorite_box);
				 convertView.setTag(holder);
			 } else {
				 holder = (ViewHolder) convertView.getTag();
			 }

			 String  m = my_item[position];
			 String rate = my_rate[position];
			 /*
			 Spanned x = Html.fromHtml(m[0]);
			 LOG("fav pos=" + position + ": " + x + "\n");
			 */

			 holder.text.setText(m);
			 holder.text.setBackgroundColor(Color.parseColor(m_list_color[0]));
			 holder.text.setTextColor(Color.parseColor(m_list_color[1]));

			 holder.rate.setText(rate);
			 holder.rate.setBackgroundColor(Color.parseColor(m_list_color[0]));
			 holder.rate.setTextColor(Color.parseColor(m_list_color[1]));

			 if (my_flag == 0) {
				 holder.box.setVisibility(View.GONE);
			 } else {
				 holder.box.setVisibility(View.VISIBLE);
			 }

			 return convertView;
		 }

	 }

	 private class ViewHolder
	 {
		 TextView text;
		 TextView rate;
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
	public native int WikiGetFullScreenFlag();

	public native String [] WikiFavorite();
	public native String [] WikiFavoriteRate();
	public native int WikiFavoriteClean();
	public native int WikiFavoriteDelete(int idx);

	public native String [] WikiListColor();
}
