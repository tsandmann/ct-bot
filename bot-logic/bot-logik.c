/*
 * c't-Bot
 * 
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your
 * option) any later version. 
 * This program is distributed in the hope that it will be 
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
 * PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public 
 * License along with this program; if not, write to the Free 
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 * 
 */

/*! @file 	bot-logik.c
 * @brief 	High-Level Routinen fuer die Steuerung des c't-Bots.
 * Diese Datei sollte der Einstiegspunkt fuer eigene Experimente sein, 
 * den Roboter zu steuern.
 * 
 * bot_behave() arbeitet eine Liste von Verhalten ab. 
 * Jedes Verhalten kann entweder absolute Werte setzen, dann kommen niedrigerpriorisierte nicht mehr dran.
 * Alternativ dazu kann es Modifikatoren aufstellen, die bei niedriger priorisierten angewendet werden.
 * bot_behave_init() baut diese Liste auf.
 * Jede Verhaltensfunktion bekommt einen Verhaltensdatensatz uebergeben, in den Sie ihre Daten eintraegt
 * 
 * @author 	Benjamin Benz (bbe@heise.de)
 * @author 	Christoph Grimmer (c.grimmer@futurio.de)
 * @date 	01.12.05
*/


#include "bot-logic/bot-logik.h"

#ifdef BEHAVIOUR_AVAILABLE

#include "display.h"
#include "rc5.h"



#include <stdlib.h>

int16 speedWishLeft;				/*!< Puffervariablen fuer die Verhaltensfunktionen absolut Geschwindigkeit links*/
int16 speedWishRight;				/*!< Puffervariablen fuer die Verhaltensfunktionen absolut Geschwindigkeit rechts*/

float faktorWishLeft;				/*!< Puffervariablen fuer die Verhaltensfunktionen Modifikationsfaktor links*/
float faktorWishRight;				/*!< Puffervariablen fuer die Verhaltensfunktionen Modifikationsfaktor rechts */

int16 target_speed_l=BOT_SPEED_STOP;	/*!< Sollgeschwindigkeit linker Motor - darum kuemmert sich bot_base()*/
int16 target_speed_r=BOT_SPEED_STOP;	/*!< Sollgeschwindigkeit rechter Motor - darum kuemmert sich bot_base() */


/*! Liste mit allen Verhalten */
Behaviour_t *behaviour = NULL;

/*! 
 * Das einfachste Grundverhalten 
 * @param *data der Verhaltensdatensatz
 */
void bot_base_behaviour(Behaviour_t *data){
	speedWishLeft=target_speed_l;
	speedWishRight=target_speed_r;
}

/*!
 * Initialisert das ganze Verhalten
 */
void bot_behave_init(void){
	#ifdef BEHAVIOUR_REMOTECALL_AVAILABLE
		// Dieses Verhalten kann andere Starten
		insert_behaviour_to_list(&behaviour, new_behaviour(254, bot_remotecall_behaviour,INACTIVE));
	#endif

	#ifdef BEHAVIOUR_SERVO_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(253, bot_servo_behaviour,INACTIVE));
	#endif

	// Demo-Verhalten, ganz einfach, inaktiv
	// Achtung, im Moment hat es eine hoehere Prioritaet als die Gefahrenerkenner!!!
	#ifdef BEHAVIOUR_SIMPLE_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(252, bot_simple_behaviour,INACTIVE));
		insert_behaviour_to_list(&behaviour, new_behaviour(251, bot_simple2_behaviour,INACTIVE));
	#endif


	// Hoechste Prioritate haben die Notfall Verhalten

	// Verhalten zum Schutz des Bots, hohe Prioritaet, Aktiv
	#ifdef BEHAVIOUR_AVOID_BORDER_AVAILABLE	
		insert_behaviour_to_list(&behaviour, new_behaviour(250, bot_avoid_border_behaviour,ACTIVE));
	#endif
	#ifdef BEHAVIOUR_AVOID_COL_AVAILABLE	
		insert_behaviour_to_list(&behaviour, new_behaviour(249, bot_avoid_col_behaviour,ACTIVE));
	#endif

	#ifdef BEHAVIOUR_SCAN_AVAILABLE
		// Verhalten, das die Umgebung des Bots on-the fly beim fahren scannt
		insert_behaviour_to_list(&behaviour, new_behaviour(155, bot_scan_onthefly_behaviour,ACTIVE));
	
		// Verhalten, das einmal die Umgebung des Bots scannt
		insert_behaviour_to_list(&behaviour, new_behaviour(152, bot_scan_behaviour,INACTIVE));
	#endif
	
	// Alle Hilfsroutinen sind relativ wichtig, da sie auch von den Notverhalten her genutzt werden
	// Hilfsverhalten, die Befehle von Boten-Funktionen ausfuehren, erst inaktiv, werden von Boten aktiviert	
	#ifdef BEHAVIOUR_TURN_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(150, bot_turn_behaviour,INACTIVE));
	#endif
	#ifdef BEHAVIOUR_DRIVE_DISTANCE_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(149, bot_drive_distance_behaviour,INACTIVE));
	#endif
	#ifdef BEHAVIOUR_GOTO_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(148, bot_goto_behaviour,INACTIVE));
	#endif

	// Hilfsverhalten zum Anfahren von Positionen
	#ifdef BEHAVIOUR_GOTOXY_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(147, bot_gotoxy_behaviour,INACTIVE));
	#endif


	#ifdef BEHAVIOUR_CATCH_PILLAR_AVAILABLE
		insert_behaviour_to_list(&behaviour, new_behaviour(44, bot_catch_pillar_behaviour,INACTIVE));
	#endif

	
	#ifdef BEHAVIOUR_OLYMPIC_AVAILABLE
		bot_olympic_init(52,100,INACTIVE);
	#endif

	#ifdef BEHAVIOUR_FOLLOW_LINE_AVAILABLE
		// Verhalten um einer Linie zu folgen
		insert_behaviour_to_list(&behaviour, new_behaviour(70, bot_follow_line_behaviour, ACTIVE));
	#endif

	#ifdef BEHAVIOUR_SOLVE_MAZE_AVAILABLE
		bot_solve_maze_init(100,43,INACTIVE);
	#endif

	#ifdef BEHAVIOUR_DRIVE_SQUARE_AVAILABLE
		// Demo-Verhalten, etwas komplexer, inaktiv
		insert_behaviour_to_list(&behaviour, new_behaviour(51, bot_drive_square_behaviour,INACTIVE));
	#endif



	// Grundverhalten, setzt aeltere FB-Befehle um, aktiv
	insert_behaviour_to_list(&behaviour, new_behaviour(2, bot_base_behaviour, ACTIVE));

	// Um das Simple-Behaviour zu nutzen, die Kommentarzeichen vor der folgenden Zeile entfernen!!!
	// activateBehaviour(bot_simple_behaviour);
	// activateBehaviour(bot_simple2_behaviour);

//	#ifdef PC
//		#ifdef DISPLAY_AVAILABLE
//			/* Anzeigen der geladenen Verhalten  */
//				Behaviour_t	*ptr	= behaviour;
//	
//				display_cursor(5,1);
//				display_printf("Verhaltensstack:\n");
//				while(ptr != NULL)	{
//					display_printf("Prioritaet: %d.\n", ptr->priority);
//					ptr = ptr->next;
//				}
//		#endif
//	#endif
}


/*!
 * Aktiviert eine Regel mit gegebener Funktion
 * @param function Die Funktion, die das Verhalten realisiert.
 */
void activateBehaviour(BehaviourFunc function){
	Behaviour_t *job;						// Zeiger auf ein Verhalten

	// Einmal durch die Liste gehen, bis wir den gewuenschten Eintrag haben 
	for (job = behaviour; job; job = job->next) {
		if (job->work == function) {
			job->active = ACTIVE;
			break;
		}
	}
}


/*!
 * Deaktiviert eine Regel mit gegebener Funktion
 * @param function Die Funktion, die das Verhalten realisiert.
 */
void deactivateBehaviour(BehaviourFunc function){
	Behaviour_t *job;						// Zeiger auf ein Verhalten
		
	// Einmal durch die Liste gehen, bis wir den gewuenschten Eintrag haben 
	for (job = behaviour; job; job = job->next) {
		if (job->work == function) {
			job->active = INACTIVE;
			break;
		}
	}
}

/*! 
 * Ruft ein anderes Verhalten auf und merkt sich den Ruecksprung 
 * return_from_behaviour() kehrt dann spaeter wieder zum aufrufenden Verhalten zurueck
 * @param from aufrufendes Verhalten
 * @param to aufgerufenes Verhalten
 * @param override Hier sind zwei Werte Moeglich:
 * 		1. OVERRIDE : Das Zielverhalten to wird aktiviert, auch wenn es noch aktiv ist. 
 * 					  Das Verhalten, das es zuletzt aufgerufen hat wird dadurch automatisch 
 * 					  wieder aktiv und muss selbst sein eigenes Feld subResult auswerten, um zu pruefen, ob das
 * 					  gewuenschte Ziel erreicht wurde, oder vorher ein Abbruch stattgefunden hat. 
 * 		2. NOOVERRIDE : Das Zielverhalten wird nur aktiviert, wenn es gerade nichts zu tun hat.
 * 						In diesem Fall kann der Aufrufer aus seinem eigenen subResult auslesen,
 * 						ob seibem Wunsch Folge geleistet wurde.
 */ 
void switch_to_behaviour(Behaviour_t * from, void *to, uint8 override ){
	Behaviour_t *job;						// Zeiger auf ein Verhalten
	
	// Einmal durch die Liste gehen, bis wir den gewuenschten Eintrag haben 
	for (job = behaviour; job; job = job->next) {
		if (job->work == to) {
			break;		// Abbruch der Schleife, job zeigt nun auf die Datenstruktur des Zielverhaltens
		}
	}	

	if (job->caller){		// Ist das auzurufende Verhalten noch beschaeftigt?
		if (override==NOOVERRIDE){	// nicht ueberschreiben, sofortige Rueckkehr
			if (from)
				from->subResult=SUBFAIL;
			return;
		}
		// Wir wollen also ueberschreiben, aber nett zum alten Aufrufer sein und ihn darueber benachrichtigen
		job->caller->active=ACTIVE;	// alten Aufrufer reaktivieren
		job->caller->subResult=SUBFAIL;	// er bekam aber nicht das gewuenschte Resultat
	}

	if (from) {
		// laufendes Verhalten abschalten
		from->active=INACTIVE;
		from->subResult=SUBRUNNING;
	}
		
	// neues Verhalten aktivieren
	job->active=ACTIVE;
	// Aufrufer sichern
	job->caller =  from;
}

/*! 
 * Kehrt zum aufrufenden Verhalten zurueck
 * @param running laufendes Verhalten
 */ 
void return_from_behaviour(Behaviour_t * data){
	data->active=INACTIVE; 				// Unterverhalten deaktivieren
	if (data->caller){			
		data->caller->active=ACTIVE; 	// aufrufendes Verhalten aktivieren
		data->caller->subResult=SUBSUCCESS;	// Unterverhalten war erfolgreich
	}
	data->caller=NULL;				// Job erledigt, Verweis loeschen
}

/*!
 * Deaktiviert alle Verhalten bis auf Grundverhalten. Bei Verhaltensauswahl werden die Aktivitaeten vorher
 * in die Verhaltens-Auswahlvariable gesichert.
 */
void deactivateAllBehaviours(void){
	Behaviour_t *job;						// Zeiger auf ein Verhalten
	
	#ifdef DISPLAY_BEHAVIOUR_AVAILABLE
	  // bei Verhaltensanzeige in Aktivitaets-Auswahl-Variable sichern
	  #ifndef DISPLAY_DYNAMIC_BEHAVIOUR_AVAILABLE
	     // bei Verhaltensanzeige in Aktivitaets-Auswahl-Variable sichern
	     // nicht bei dynamischer Anzeige und Selektion
        set_behaviours_equal();
      #endif
	#endif
		
	// Einmal durch die Liste gehen und (fast) alle deaktivieren, Grundverhalten nicht 
	for (job = behaviour; job; job = job->next) {
		if ((job->priority >= PRIO_VISIBLE_MIN) &&(job->priority <= PRIO_VISIBLE_MAX)) {
            // Verhalten deaktivieren 
			job->active = INACTIVE;	
		}
	}	
}

/*! 
 * Zentrale Verhaltens-Routine, wird regelmaessig aufgerufen. 
 * Dies ist der richtige Platz fuer eigene Routinen, um den Bot zu steuern.
 */
void bot_behave(void){	
	Behaviour_t *job;						// Zeiger auf ein Verhalten
	
	float faktorLeft = 1.0;					// Puffer fuer Modifkatoren
	float faktorRight = 1.0;				// Puffer fuer Modifkatoren
	
	#ifdef RC5_AVAILABLE
		rc5_control();						// Abfrage der IR-Fernbedienung
	#endif

	/* Solange noch Verhalten in der Liste sind...
	   (Achtung: Wir werten die Jobs sortiert nach Prioritaet aus. Wichtige zuerst einsortieren!!!) */
	for (job = behaviour; job; job = job->next) {
		if (job->active) {
			/* WunschVariablen initialisieren */
			speedWishLeft = BOT_SPEED_IGNORE;
			speedWishRight = BOT_SPEED_IGNORE;
			
			faktorWishLeft = 1.0;
			faktorWishRight = 1.0;
			
			job->work(job);	/* Verhalten ausfuehren */
			/* Modifikatoren sammeln  */
			faktorLeft  *= faktorWishLeft;
			faktorRight *= faktorWishRight;
           /* Geschwindigkeit aendern? */
			if ((speedWishLeft != BOT_SPEED_IGNORE) || (speedWishRight != BOT_SPEED_IGNORE)){
				if (speedWishLeft != BOT_SPEED_IGNORE)
					speedWishLeft *= faktorLeft;
				if (speedWishRight != BOT_SPEED_IGNORE)
					speedWishRight *= faktorRight;
					
				motor_set(speedWishLeft, speedWishRight);
				break;						/* Wenn ein Verhalten Werte direkt setzen will, nicht weitermachen */
			}
			
		}
		/* Dieser Punkt wird nur erreicht, wenn keine Regel im System die Motoren beeinflusen will */
		if (job->next == NULL) {
				motor_set(BOT_SPEED_IGNORE, BOT_SPEED_IGNORE);
		}
	}
}

/*! 
 * Erzeugt ein neues Verhalten 
 * @param priority Die Prioritaet
 * @param *work Den Namen der Funktion, die sich drum kuemmert
 */
Behaviour_t *new_behaviour(uint8 priority, void (*work) (struct _Behaviour_t *data), int8 active){
	Behaviour_t *newbehaviour = (Behaviour_t *) malloc(sizeof(Behaviour_t)); 
	
	if (newbehaviour == NULL) 
		return NULL;
	
	newbehaviour->priority = priority;
	newbehaviour->active=active;
	newbehaviour->next= NULL;
	newbehaviour->work=work;
	newbehaviour->caller=NULL;
	newbehaviour->subResult=SUBSUCCESS;
	return newbehaviour;
}

/*!
 * Fuegt ein Verhalten der Verhaltenliste anhand der Prioritaet ein.
 * @param list Die Speicherstelle an der die globale Verhaltensliste anfaengt
 * @param behave Einzufuegendes Verhalten
 */
void insert_behaviour_to_list(Behaviour_t **list, Behaviour_t *behave){
	Behaviour_t	*ptr	= *list;
	Behaviour_t *temp	= NULL;
	
	/* Kein Eintrag dabei? */
	if (behave == NULL)
		return;
	
	/* Erster Eintrag in der Liste? */
	if (ptr == NULL){
		ptr = behave;
		*list = ptr;
	} else {
		/* Gleich mit erstem Eintrag tauschen? */
		if (ptr->priority < behave->priority) {
			behave->next = ptr;
			ptr = behave;
			*list = ptr;
		} else {
			/* Mit dem naechsten Eintrag vergleichen */
			while(NULL != ptr->next) {
				if (ptr->next->priority < behave->priority)	
					break;
				
				/* Naechster Eintrag */
				ptr = ptr->next;
			}
			
			temp = ptr->next;
			ptr->next = behave;
			behave->next = temp;
		}
	}
}


//TODO:	Code optimieren und pruefen! => Horror...	
#ifdef DISPLAY_BEHAVIOUR_AVAILABLE
	/* ermittelt ob noch eine weitere Verhaltensseite existiert */ 	
	int8  another_behaviour_page(void) {
		int16 max_behaviours ;
		Behaviour_t	*ptr	;
	  
	  	// TODO!
	  	/* dazu muss ich auch gueltige Screenseite sein */
	  	#ifdef DISPLAY_SCREENS_AVAILABLE
	   		if (display_screen != 2) return 0;
	  	#endif 
	  	ptr = behaviour;
	  	max_behaviours = 0;
	  
		// zuerst alle Verhalten ermitteln ausser Grundverhalten
	  	while(ptr != NULL)	{			 
			if ((ptr->priority >= PRIO_VISIBLE_MIN) &&(ptr->priority <= PRIO_VISIBLE_MAX)) max_behaviours++;		  							  
			ptr = ptr->next;
	   }  
	   return (behaviour_page  * 6) < max_behaviours;
	}
	
	/*! 
	 * toggled ein Verhalten der Verhaltensliste an Position pos,
	 * die Aenderung erfolgt nur auf die Puffervariable  
	 * @param pos Listenposition, entspricht der Taste 1-6 der gewaehlten Verhaltensseite
	 */
	void toggleNewBehaviourPos(int8 pos){
		Behaviour_t *job;						// Zeiger auf ein Verhalten
	    int8 i;
	    
	    // nur aendern, wenn ich richtige Screenseite bin 
	     if (display_screen != 2) return;
	     
	    // richtigen Index je nach Seite ermitteln 
	    pos = (behaviour_page - 1) * 6 + pos;
	    i   = 0;
	
		// durch die Liste gehen, bis wir den gewuenschten Index erreicht haben 
		for (job = behaviour; job; job = job->next) {
			if ((job->priority >= PRIO_VISIBLE_MIN) &&(job->priority <= PRIO_VISIBLE_MAX)) {		
				i++;
			  	if (i == pos) {
			  		// bei dynamischer Wahl wird direkt die Zustandsvariable geaendert
			  	  	#ifdef DISPLAY_DYNAMIC_BEHAVIOUR_AVAILABLE
			  	    	job->active = !job->active;
			  	  	#else
			  	    	job->active_new = !job->active_new;
			  	  	#endif		      
				  	break;
			  	}
		    }
		}
	}
	
	#ifndef DISPLAY_DYNAMIC_BEHAVIOUR_AVAILABLE
		/*! 
		 * Startschuss, die gewaehlten neuen Verhaltensaktivitaeten werden in die
		 * Verhaltensliste geschrieben und die Verhalten damit scharf geschaltet 
		 */
		void set_behaviours_active_to_new(void) {
			Behaviour_t *job;	
			for (job = behaviour; job; job = job->next){
				if ((job->priority >= PRIO_VISIBLE_MIN) &&(job->priority <= PRIO_VISIBLE_MAX)) job->active = job->active_new;            				 
			}
		}
		
		/*!
		 * Die Aktivitaeten der Verhalten werden in die Puffervariable geschrieben, 
		 * welche zur Anzeige und Auswahl verwendet wird
		 */
		void set_behaviours_equal(void) {
			Behaviour_t *job;	
			for (job = behaviour; job; job = job->next){
				if ((job->priority >= PRIO_VISIBLE_MIN) &&(job->priority <= PRIO_VISIBLE_MAX)) job->active_new = job->active;            				 
		   	}
		}
	#endif	// DISPLAY_DYNAMIC_BEHAVIOUR_AVAILABLE
	
	/*!
	 * @brief	Zeigt Informationen ueber Verhalten an 
	 * (aus ct-bot.c)
	 */
	void behaviour_display(void){		
        /* Definitionen fuer die Verhaltensanzeige */
		Behaviour_t	*ptr	= behaviour;
		int8 colcounter       = 0;
		int8 linecounter      = 0;
		int8 firstcol         = 0; 

		display_cursor(1,1);  
         /* zeilenweise Anzeige der Verhalten */
		display_printf("Verhalten (Pri/Akt)%d",behaviour_page);
		
		colcounter = 0; 
		linecounter = 2;
		/* je nach Seitenwahl die ersten  Saetze ueberlesen bis richtige Seite */
		firstcol = (behaviour_page -1)*6;
		 
		 /* max. 3 Zeilen mit 6 Verhalten anzeigbar wegen Ueberschrift Seitensteuerung bei mehr Verhalten */ 
		while((ptr != NULL)&& (linecounter<5))	{
			if  ((ptr->priority >= PRIO_VISIBLE_MIN) &&(ptr->priority <= PRIO_VISIBLE_MAX)) {
            	if   (colcounter >= firstcol) { 
	          		display_cursor(linecounter,((colcounter % 2)* 12)+1);
					#ifdef DISPLAY_DYNAMIC_BEHAVIOUR_AVAILABLE
		        		display_printf(" %3d,%2d",ptr->priority,ptr->active);
		      		#else
		        		display_printf(" %3d,%2d",ptr->priority,ptr->active_new);				      
		      		#endif
		      		colcounter++;
		    
		      		/* bei colcounter 0 neue Zeile */
		      		if (colcounter % 2 == 0) linecounter++;		      
		    	} else colcounter++;
		  	}
			ptr = ptr->next;
		}
	}  
#endif	// DISPLAY_BEHAVIOUR_AVAILABLE
#endif	// BEHAVIOUR_AVAILABLE
