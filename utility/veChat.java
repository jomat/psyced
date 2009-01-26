/*
** veChat - applet for the psyced
** written 1997 by Carlo v. Loesch for symlynX.com
**
** http://www.psyced.org/
** http://symlynX.com/
**
** basics seen in JavaTalkClient.java by
** Jean-Guy Speton and Maharajapuram Balakrishnan
** http://www.cs.orst.edu/~speton/
*/

/* Let's import precisely what we need to improve compilation times */
import java.awt.event.*; // good start! TODO
import java.awt.Font;
import java.awt.Container;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.TextField;
import java.awt.Component;
//import java.awt.Insets;
//import java.awt.Graphics;
import java.awt.Color;
import java.awt.Menu;
import java.awt.MenuBar;
import java.awt.MenuItem;
import java.awt.Label;
import java.awt.TextArea;
import java.awt.List;
import java.awt.Button;
import java.awt.FontMetrics;
//import java.awt.Dimension;
import java.awt.Choice;
import java.awt.Frame;
import java.awt.BorderLayout;
import java.io.DataInputStream;

//import java.io.BufferedReader;
//import java.io.InputStreamReader;
import java.awt.Color;
import java.util.HashMap;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.EOFException;
import java.util.StringTokenizer;
import java.net.Socket;
import java.net.URL;
import java.applet.Applet;

public class veChat extends Applet implements ActionListener {
    veChatFrame win = null;
    public Button butt;

    public void init() {
	win = new veChatFrame(this);
	String actiText, closeText;

	setLayout(new BorderLayout());

	Color col = win.getColorParameter("BGCOLOR");
	if (col != null) this.setBackground(col);
	col = win.getColorParameter("FGCOLOR");
	if (col != null) this.setForeground(col);

	actiText = win.getParameter("ACTIVATION");
	if (actiText == null) actiText = "CLICK HERE TO JOIN THE CHAT";
	butt = new Button(actiText);
	col = win.getColorParameter("BGLAUNCHER");
	butt.setBackground(col);
	col = win.getColorParameter("FGLAUNCHER");
	butt.setForeground(col);
	add("Center",butt);
	butt.addActionListener(this);
	
	butt.setLabel("CLICK HERE TO EXIT THE CHAT");
	win.doInit();
	
	    //if (nick == null && input.getText().equals(defNick)) return;
	    // doConnect(host, port, nick, pass);
    }

    public void actionPerformed(ActionEvent event) {
    	if(win.isVisible()) {
			win.close();
			butt.setLabel("CLICK HERE TO JOIN THE CHAT");
    	} else {
    		win.doShow();
			butt.setLabel("CLICK HERE TO EXIT THE CHAT");
    	}
	}
	
    public String getAppletInfo() { return "veChat GUI 2.0 by CvL@vapl.symlynX.com"; }
}

class veChatFrame extends Frame implements Runnable, WindowListener, 
														ActionListener {
    veChat app = null;
    boolean isVisible = false;
	Language l = new Language();

    public String getAppletInfo() { return "veChat GUI 2.0 by CvL@vapl.symlynX.com"; }

    public String getParameter(String name) {
	if (app != null) return app.getParameter(name);
	return null;
    }
    public URL getDocumentBase() {
	if (app != null) return app.getDocumentBase();
	return null;
    }

    public veChatFrame(veChat parent) {
	app = parent;
	setSize(500,400);
	String t = getParameter("TITLE");
	setTitle(t != null ? t : getAppletInfo());
    }

    public void doShow() {
		if (isVisible()) show();
		else takeOver();
		input.requestFocus();
    }

    public void takeOver() {
    MenuItem menuItem;
	menuBar = new MenuBar();
	menu = new Menu("Net");
	
	menuItem = new MenuItem("Start");
	menuItem.addActionListener(this);
	menu.add(menuItem);
	
	menuItem = new MenuItem("Stop");
	menuItem.addActionListener(this);
	menu.add(menuItem);
	
	menu.addSeparator();
	
	menuItem = new MenuItem("End");
	menuItem.addActionListener(this);
	menu.add(menuItem);
	
	menuBar.add(menu);

	menu = new Menu("Font");
	for (int i=6; i <= 24; i+=2) {
		menuItem = new MenuItem(String.valueOf(i));
		menuItem.addActionListener(this);
		menu.add(menuItem);
	}
	menuBar.add(menu);
	
	menu = new Menu("Language");
	
	menuItem = new MenuItem("english");
	menuItem.addActionListener(this);
	menu.add(menuItem);
	
	menuItem = new MenuItem("german");
	menuItem.addActionListener(this);
	menu.add(menuItem);
	
	menuBar.add(menu);

	menu = new Menu("People");
	
	menuItem = new MenuItem("/people");
	menuItem.addActionListener(this);
	menu.add(menuItem);
	
	
	menuItem = new MenuItem("/who");
	menuItem.addActionListener(this);
	menu.add(menuItem);
	
	menu.addSeparator();
	
	menuItem = new MenuItem("/show");
	menuItem.addActionListener(this);
	menu.add(menuItem);
		
	menuBar.add(menu);
	menu = new Menu("Room");
	
	menuItem = new MenuItem("/users");
	menuItem.addActionListener(this);
	menu.add(menuItem);
	
	menuItem = new MenuItem("/status");
	menuItem.addActionListener(this);
	menu.add(menuItem);	

	String placeList = getParameter("PLACES");
	if (placeList != null) {
	    StringTokenizer rot = new StringTokenizer(placeList, ";");

	    menu = new Menu("Places");
	    while (rot.hasMoreTokens()){
			String t = rot.nextToken();
			if (t == "-") menu.addSeparator();
			else {
				menuItem = new MenuItem(t);
				menuItem.addActionListener(this);
				menu.add(menuItem);	
			} 
	    }
	    menuBar.add(menu);
	}
	menu = new Menu("Info");
	
	menuItem = new MenuItem("Help");
	menuItem.addActionListener(this);
	menu.add(menuItem);
	
	menuItem = new MenuItem("About");
	menuItem.addActionListener(this);
	menu.add(menuItem);
	
//	menu.addSeparator();
//	menu.add("Register");
	menu.addSeparator();
	
	menuItem = new MenuItem("/edit");
	menuItem.addActionListener(this);
	menu.add(menuItem);
	
	// menuBar.setHelpMenu(menu);	-- this sadly doesn't work!
	menuBar.add(menu);

	setMenuBar(menuBar);
	show();
	if (!isVisible) {
	    if (connected()) say("\n"+l.get(LANG, "_connection_closed")+"\n");
	    isVisible = true;
	}
	setMetrics(view);
	if (connected()) {
	    doDisconnect();
	}
	else haveDisconnected();
	greet();
    }

    String nick = null, pass = null, host = null;
    int port = PORT;

    String defNick = null, talkNick;
    String LAYOUT = null, LANG = "en", FONTFACE = null;
    Font f = null;

    protected Label prompt;
    protected List people;
    protected TextArea view;
    FontMetrics fm;
    int spaceChar, viewWidth;
    protected Choice ucmd;
    protected TextField input;
    protected TextField status;
    private MenuBar menuBar;
    private Menu menu;
    public void doInit() {
	nick = getParameter("NICK");
	pass = getParameter("PASS");
	host = getParameter("HOST"); // works only outside of sandbox
	try { port = Integer.parseInt(getParameter("PORT")); }
	catch (NumberFormatException e) { port = PORT; }

	LANG = getParameter("LANG") == null ? "en" : getParameter("LANG");
	LAYOUT = getParameter("LAYOUT");
	FONTFACE = getParameter("FONTFACE");
	if (FONTFACE != null)
	    f = new Font(FONTFACE, Font.PLAIN, 12);

	defNick = getParameter("DEFNICK");
	if (defNick == null) defNick = "";

	Color col = getColorParameter("BGCOLOR");
	if (col != null) this.setBackground(col);
	col = getColorParameter("FGCOLOR");
	if (col != null) this.setForeground(col);
	Color bcol = getColorParameter("BGBUTTON");

	this.setLayout(new GridBagLayout());

	prompt = new Label("Nickname:");
	col = getColorParameter("FGPROMPT");
	if (col != null) prompt.setForeground(col);
	constrain(this, prompt, 0, 1, 1, 1,
		GridBagConstraints.BOTH, GridBagConstraints.CENTER, 0.1, 0.0,
		6, 0, 0, 0);

	input = new TextField("");
	input.setEditable(true);
	input.addActionListener(this);
	
	constrain(this, input, 1, 1, 1, 1,
		GridBagConstraints.BOTH, GridBagConstraints.CENTER, 1.0, 0.0,
		0, 0, 4, 0);

	ucmd = new Choice();
	ucmd.addItem("");
	ucmd.addItemListener(new ucmdEvent() );
	
	col = getColorParameter("BGPERSON");
	if (col != null) ucmd.setBackground(col);
	col = getColorParameter("FGPERSON");
	if (col != null) ucmd.setForeground(col);
	constrain(this, ucmd, 2, 1, 1, 1,
		GridBagConstraints.NONE, GridBagConstraints.CENTER, 1.0, 0.0,
		0, 4, 0, 0);

	view = new TextArea("");
	constrain(this, view, 0, 0, 2, 1,
		GridBagConstraints.BOTH, GridBagConstraints.CENTER, 4.0, 1.0,
		4, 4, 4, 4);

	people = new List();
	people.addItemListener(new peopleEvent() );
	
	col = getColorParameter("BGPEOPLE");
	if (col != null) people.setBackground(col);
	col = getColorParameter("FGPEOPLE");
	if (col != null) people.setForeground(col);
	constrain(this, people, 2, 0, 1, 1,
		GridBagConstraints.BOTH, GridBagConstraints.CENTER, 0.2, 1.0,
		0, 18, 4, 4);
	//try { setMetrics(view); }
	//catch (NullPointerException e) { /* do nothing ... */ }
	
	view.setEditable(false);
	col = getColorParameter("BGVIEW");
	if (col != null) view.setBackground(col);
	//col = new Color(0,0,0);
	col = getColorParameter("FGCOLOR");
	view.setForeground(col);
	//System.out.println(col);

	status = new TextField("");
	status.setEditable(false);
	col = getColorParameter("BGSTATUS");
	if (col != null) status.setBackground(col);
	col = getColorParameter("FGSTATUS");
	if (col != null) status.setForeground(col);
	constrain(this, status, 0, 2, 3, 1,
		GridBagConstraints.BOTH, GridBagConstraints.CENTER, 1.0, 0.0,
		4, 4, 4, 4);
	col = getColorParameter("BGINPUT");
	if (col != null) input.setBackground(col);
	col = getColorParameter("FGINPUT");
	if (col != null) input.setForeground(Color.white);
	// g = getGraphics();
	if (host == null) host = app.getCodeBase().getHost();
	takeOver();
    }

    public void init() {
	if (host == null) doInit();
	// repaint();
	// input.selectAll();
    }

    /** Einfache Bildschirmausgabefunktion */
    public void say(String msg) {
	synchronized (view) {
	    view.append(msg);
	}
    }

    public void setMetrics(TextArea vu) {
	fm = vu.getFontMetrics(vu.getFont());
	spaceChar = fm.stringWidth(" ");
	viewWidth = vu.getSize().width;
    }

    /** Smarte Bildschirmausgabefunktion */
    public void wrapsay(String t) {
    int sw, line = 33;	    // empirisch ermittelt
	String token, msg = "";
	StringTokenizer st = new StringTokenizer(t);

	while (st.hasMoreTokens()) {
	    token = st.nextToken();
	    sw = fm.stringWidth(token);
	    line += sw + spaceChar;
	    if (line < viewWidth || sw > viewWidth) {
		msg += token + " ";
	    } else {
		    // indent, a bit like ircII's /set indent on
		    // guess who coded that one...  ;)
		    //
		msg += "\n   " + token + " ";
		line = 33 + 4*spaceChar + sw;
	    }
	}
	say(msg + "\n");
    }


    public void constrain(Container container, Component component,
			    int grid_x, int grid_y,
			    int grid_width, int grid_height,
			    int fill, int anchor, double weight_x, double weight_y,
			    int left, int top, int right, int bottom)
    {
	GridBagConstraints c = new GridBagConstraints();
	c.gridx = grid_x; c.gridy = grid_y;
	c.gridwidth = grid_width; c.gridheight = grid_height;
	c.fill = fill; c.anchor = anchor;
	c.weightx = weight_x; c.weighty = weight_y;

	if (f != null) component.setFont(f);
	((GridBagLayout)container.getLayout()).setConstraints(component, c);
	container.add(component);
    }

    protected Color getColorParameter(String name) {
	String value = this.getParameter(name);
	//System.err.println("value: "+value);
	if (value != null && value.length() == 7)
	    value = value.substring(1);
	int intvalue;
	try { intvalue = Integer.parseInt(value, 16); }
	catch (NumberFormatException e) { return null; }
	return new Color(intvalue);
    }

    public void greet() {
	view.setText("");
	String g = getParameter("GREETING");
	if (g != null) {
	    StringTokenizer st = new StringTokenizer(g, ";");
	    while (st.hasMoreTokens()) {
		wrapsay(st.nextToken() + "\n");
	    }
	}
	say(getAppletInfo());
	say("\n"+l.get(LANG, "_symlynx")+"\n");
    }

    public void doTalk(String sel) {
	if (talkNick == sel || sel == null) {
	    send("/talk\n");
	    talkNick = null;
	    prompt.setText(l.get(LANG, "_to_everybody"));
	 // Color col = getColorParameter("FGCOLOR");
	 // if (col != null) view.setForeground(col);
	    Color col = getColorParameter("BGVIEW");
	    if (col != null) view.setBackground(col);
	} else {
	    talkNick = sel;
	    send("/talk "+ talkNick +"\n");
	    prompt.setText(talkNick +":");
	 // Color col = getColorParameter("FGTALK");
	 // if (col != null) view.setForeground(col);
	    Color col = getColorParameter("BGTALK");
	    if (col != null) view.setBackground(col);
	}
    }

	/********************************************************************/
	/********************* start of eventhandling ***********************/
	/********************************************************************/
	
	public void windowActivated(WindowEvent evt)  { } 
	public void windowClosing(WindowEvent evt)  { windowClosed(evt); }
	public void windowDeactivated(WindowEvent evt)  { }
	public void windowDeiconified(WindowEvent evt)  { }
	public void windowIconified(WindowEvent evt)  { }
	public void windowOpened(WindowEvent evt)  { }
	
	public void windowClosed(WindowEvent event) {
		doDisconnect();
		dispose();
		isVisible = false;
	}

	public class ucmdEvent implements ItemListener {
		public void itemStateChanged(ItemEvent evt) {
			Object arg = evt.getItem();
			String p = people.getSelectedItem();
			//System.out.println(arg + " and " + p + " given");

			if (p == null || p == "") {
			if (connected())
				sayStatus(l.get(LANG, "_pick_person_from_list"));
				return;
			}
			if (arg != null && ((String)arg).startsWith("/")) {
				if (arg == "/talk")
					doTalk(p);
				else send(arg +" "+ p + "\n");
				  // send(arg +" "+ p + (arg == "Notify"? " immediate\n": "\n"));
				}
				
				if(!(ucmd.getSelectedIndex() == 0)) ucmd.select(0);
	    
			return;
		}
	}
	
	public class peopleEvent implements ItemListener {
	
	public void itemStateChanged(ItemEvent evt) {
		//System.out.println("Event: "+evt.getStateChange() + " Source: " +
		//					evt.getSource() + " people: " +people);
		switch(evt.getStateChange()) {
			case ItemEvent.SELECTED:
			if (evt.getSource() == people) {
				if (ucmd.getItemCount() == 1) {
					ucmd.addItem("person");
					ucmd.addItem(" ");
					ucmd.addItem("/talk");
					ucmd.addItem("/examine");
					ucmd.addItem(" ");
					ucmd.addItem("/ignore");
					//ucmd.addItem("/reduce");
					ucmd.addItem("/show");
					//ucmd.addItem("/highlight");
					ucmd.addItem(" ");
					ucmd.addItem("/friend");
					ucmd.remove(0);
					 // ucmd.addItem(" ");
					 // ucmd.addItem("Notify");
					if(!(ucmd.getSelectedIndex() == 0)) ucmd.select(0);
				}
				
				sayStatus(l.get(LANG, "_select_function"));
			}
		
				break;
			case ItemEvent.DESELECTED:
				if (evt.getSource() == people) {
					doTalk(null);
				}
				 
		}
	}
	}
	
	public void actionPerformed(ActionEvent event) {
		status.setText("");
		input.requestFocus();

		if(event.getSource() == input) {
			if (connected()) executeInput();
			else doConnect(host, port, nick, pass);
			return;
		}
		if(event.getSource() instanceof MenuItem) {
			Object arg = (Object)event.getActionCommand();
			//System.out.println(arg);
			
			if (((String)arg).startsWith("/")) {
					send((String)arg + "\n");
					return;
			}
			if (((String)arg).equals("german")) {
				LANG = "de";
				send("/set language de\n");
				return;
			}
			if (((String)arg).equals("english")) {
				LANG = "en";
				send("/set language en\n");
				return;
			}
			if (((String)arg).equals("Start")) {
				if (!connected()) doConnect(host, port, nick, pass);
					return;
			}
			if (((String)arg).equals("Stop")) {
				send("/quit\n");
				return;
			}
			if (((String)arg).equals("End")) {
				doDisconnect();
				dispose();
				isVisible = false;
				return;
			}
			if (((String)arg).equals("About")) {
				sayStatus(getAppletInfo());
				return;
			}
			if (((String)arg).equals("Help")) {
				String t = getParameter("URLHELP");
				try {
					// "http://ve.lava.de:33333/info/"));
					app.getAppletContext().showDocument(
									new URL("http://help.pages.de/"), "_blank" );
				} catch(Exception e) {} // MalformedURLException
				return;
			}
	/*	    if (((String)arg).equals("Register")) {
				if (nick != null) {
					String t = getParameter("URLREGISTER");
					try {
					app.getAppletContext().showDocument(new URL(
						getDocumentBase(), (t != null ? t :
						("/"+LAYOUT+"/register") ) +"?user="+ nick));
					} catch(Exception e) {} // MalformedURLException
				}
				return;
			}
	*/
			Font f = view.getFont();
			try {
				int fontSize = Integer.parseInt((String)arg);
				view.setFont(new Font(f.getName(), f.getStyle(), fontSize));
			}
			catch (NumberFormatException e) {
				send("/go "+ (String)arg +"\n");
			}
			return;
		
			
			
		}
	}
	
	/********************************************************************/
	/********************* end of eventhandling ***********************/
	/********************************************************************/

    public String executeInput() {
	/* if (input.echoCharIsSet()) {
		((GridBagLayout)this.getLayout()).removeLayoutComponent(input);
		input = new TextField("");
		input.setEditable(true);
		constrain(this, input, 1, 0, 2, 1, GridBagConstraints.BOTH,
			GridBagConstraints.CENTER, 1.0, 0.0, 2, 2, 2, 2);
		// input.setEchoChar('\0');
		show();
	} */
	if (input.getText().length() > 0) {
	    String ret = input.getText();
	    if (send(ret + "\n")) input.setText("");
	    return ret;
	}
	else {
	    view.setText("");
	    // send("/clear\n");
	}
	return "";
    }
    public void haveConnected(String ni) {
	prompt.setText(l.get(LANG, "_to_everybody"));
	// clear output window
	view.setText("");
    }

    public void haveDisconnected() {
	// clear list
	people.removeAll();
	if (input != null) {
	    prompt.setText("Nickname:");
	    if (nick == null) {
		    input.setText(defNick);
		    input.selectAll();
		    input.requestFocus();
	    } else
		input.setText(nick);
	}
	status.setText("");
    }

    public void setPeople(String list) {
	people.removeAll();
	StringTokenizer t = new StringTokenizer(list,"\t\n\r .,:;");
	while (t.hasMoreTokens()) { people.add(t.nextToken()); }
    }
    public void addPeople(String item) {
	people.add(item);
    }
    public void delPeople(String item) {
	for (int i = 0; i < people.getItemCount(); i++) {
	    if (item.equals(people.getItem(i))) {
		people.remove(i);
		break;
	    }
	}
    }
    public void sayStatus(String msg) {
	if (msg != null) status.setText(msg);
    }


    protected Socket socket = null;
//    protected BufferedReader in = null;
    protected DataInputStream in = null;
    protected DataOutputStream out = null;
    protected Thread engine;

    private static final int PORT = 2008;

    public boolean connected() {
	if (socket != null) return true;
	return false;
    }

    public void doConnect(String ho, int po, String ni, String pa) {
	try {
	    socket=	new Socket(ho, po);
	    out =	new DataOutputStream(socket.getOutputStream());
//	    in =	new BufferedReader(new InputStreamReader(socket.getInputStream()));
	    in =	new DataInputStream(socket.getInputStream());
	}
	catch (IOException e) {
	    wrapsay(l.get(LANG, "_direct_connection_error")+"("+ e +").\n");
	    socket = null;
	    in = null;
	    out = null;
	    return;
	}
	try {
	    String t = getParameter("DEFPLACE");
	    if (t != null) send("=_place "+ t +"\n");
	    if (LAYOUT != null) send("=_layout "+ LAYOUT +"\n");
	    if (LANG != null) send("=_language "+ LANG +"\n");
	    if (pa != null) send("=_password "+ pa +"\n");

	    if (input.getText().length() > 0)
	      nick = executeInput();
	    else
	      send((ni == null ? defNick : ni)+ "\n");
	} catch(Exception e) {
	    wrapsay(l.get(LANG, "_login_error")+"("+ e +").\n");
	    return;
	}
	haveConnected(ni);

	// potentially throw old thread away
	engine = new Thread(this);
	engine.start();
    }

	public void close() {
		doDisconnect();
		dispose();
		isVisible = false;
	}

    void doDisconnect() {
	try {
	    if (out != null) out.close();
	    if (in != null) in.close();
	    if (socket != null) socket.close();
	    socket = null;
	}
	catch (IOException e) {
	    say("== Stop: "+ e +"\n");
	}
	haveDisconnected();

	if (engine != null) {
	    // this statement could be executed by the thread itself
	    // therefore nothing important may follow!

	    if (engine.isAlive()) engine.stop();
	}
    }

    public boolean send(String t) {
		if (!connected()) doConnect(host, port, nick, pass);
		synchronized (out) {
		    try {
			out.writeBytes(t);
			return true;
		    }
		    catch (IOException e) {
			say("send: "+ e);
		    }
		}
		return false;
    }

    public void run() {
	try {
  	  // loop now, waiting for messages from the server
  	  while(true) {
	    char msgType;
	    String msg;

	    do msgType = (char) in.readByte();
	    while (msgType == '\n' || msgType == '\r' || msgType == ' ');
	    if (msgType != '|') {
		wrapsay(msgType + in.readLine());
	    } else {
		msgType = (char) in.readByte();
		msg = in.readLine();

		//System.err.println("type: "+msgType);
		switch (msgType) {
		case '?':
		    // askPassword() ?
		    //input.setEchoChar('*');
		    // fall thru
		case '!':
		    sayStatus(msg);
		    // wrapsay("* "+ msg);
		    break;
		case '+':
		addPeople(msg);	
		    break;
		case '-':
		delPeople(msg);
		    break;
		case '=':
		setPeople(msg);
		    break;
		//se '*':
		default:
		    wrapsay(msgType + msg); // to be renamed into "sayText" ?
		    break;
		}
	    }
	  }
	}
	catch (EOFException e) {
	    say(l.get(LANG, "_connection_closed")+"\n");
//	    say("== Verbindung beendet.\n");
	    socket = null;
	    doDisconnect();
	}
	catch (IOException e) {
	    say(l.get(LANG, "_connection_closed")+".\n (" + e.toString() +")\n");
	    doDisconnect();
	}
	try { if (in != null) { in.close(); }
	} catch (IOException e) {}
    }
}

class Language {
	private HashMap textdb;
	
	public Language() {
		textdb = new HashMap();
	
		textdb.put("en_connection_closed","== Connection closed.");
		textdb.put("en_login_error","== Error attempting to log in.");
		textdb.put("en_symlynx","PSYC - simple web chat");
		textdb.put("en_direct_connection_error","== Cannot connect directly.");
		textdb.put("en_to_everybody","To everybody:");
		textdb.put("en_pick_person_from_list","Pick a person from the list first, please.");
		textdb.put("en_select_function","Pick a function from the selector box.");
	
		textdb.put("de_connection_closed","== Verbindung beendet.");
		textdb.put("de_login_error","== Fehler beim Einloggen.");
		textdb.put("de_symlynx","PSYC - einfacher Webchat");	
		textdb.put("de_direct_connection_error","== Direktverbindung klappt nicht.");
		textdb.put("de_to_everybody","An alle:");
		textdb.put("de_pick_person_from_list","Wähle zuerst eine Person aus der Liste.");
		textdb.put("de_select_function","Wähle eine Funktion aus der Auswahlbox.");
	}
	
	public String get(String lang, String code) {
		if (textdb.get(lang+code) != "") {
			return (String)textdb.get(lang+code);
		} else {
			return "Error accessing Textdb.";
		}
	}
}
