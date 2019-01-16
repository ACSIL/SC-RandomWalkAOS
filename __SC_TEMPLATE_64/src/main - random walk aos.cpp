#include "sierrachart.h"
#include<random>
#include<chrono>
#include<string>

SCDLLName("RANDOM WALK AOS")

unsigned int rn_generator(SCStudyInterfaceRef sc)
{
	std::mt19937 gen;
	gen.seed(static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
	std::uniform_int_distribution<int> distribution(sc.Input[10].GetInt(), sc.Input[11].GetInt()); //set min do max to define trading frequency - the higher the max number, the less entries within a day
	return distribution(gen);
}

unsigned char ls_generator()
{
	std::mt19937 gen;
	gen.seed(static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
	std::uniform_int_distribution<int> distribution(0, 1); //generate 1/0 to decide for long/short
	return distribution(gen);
}

bool is_rth(SCStudyInterfaceRef sc)
{
	if (sc.BaseDateTimeIn[sc.Index].GetTime() > sc.Input[1].GetTime() && sc.BaseDateTimeIn[sc.Index].GetTime() < sc.Input[2].GetTime())
		return 1;
	else
		return 0;
}

SCSFExport scsf_random_walk_aos(SCStudyInterfaceRef sc)
{
	SCInputRef display_values = sc.Input[0];
	SCInputRef rth_start = sc.Input[1];
	SCInputRef rth_finish = sc.Input[2];
	SCInputRef flat_time = sc.Input[3];

	if (sc.SetDefaults)
	{
		rth_start.Name = "Trade From:";
		rth_start.SetTime(HMS_TIME(8, 30, 0));
		rth_finish.Name = "Trade Till:";
		rth_finish.SetTime(HMS_TIME(15, 00, 00));
		flat_time.Name = "Flat At:";
		flat_time.SetTime(HMS_TIME(15, 10, 00));
	
		display_values.Name = "Display Values";
		display_values.SetYesNo(1);

		sc.Subgraph[0].Name = "RTH start";
		sc.Subgraph[0].DrawStyle = DRAWSTYLE_ARROW_UP;
		sc.Subgraph[0].PrimaryColor = RGB(0, 255, 0);
		sc.Subgraph[0].SecondaryColor = RGB(255, 0, 0);
		sc.Subgraph[0].SecondaryColorUsed = 1;
		sc.Subgraph[0].LineWidth = 10;

		sc.Input[10].Name = "Random numbers range - min";
		sc.Input[10].SetInt(10);
		sc.Input[11].Name = "Random numbers range - max";
		sc.Input[11].SetInt(3600);

		sc.GraphName = "Randow walk AOS";
		sc.AutoLoop = 1;
		sc.GraphRegion = 0;
		sc.UpdateAlways = 1;
		sc.FreeDLL = 1;

		return;
	}
	sc.AllowMultipleEntriesInSameDirection = true;
	sc.MaximumPositionAllowed = 1000;
	sc.SupportReversals = false;
	sc.SendOrdersToTradeService = false;
	sc.AllowOppositeEntryWithOpposingPositionOrOrders = true;
	sc.SupportAttachedOrdersForTrading = true;
	sc.CancelAllOrdersOnEntriesAndReversals = false;
	sc.AllowEntryWithWorkingOrders = false;
	sc.CancelAllWorkingOrdersOnExit = false;
	sc.AllowOnlyOneTradePerBar = true;
	sc.MaintainTradeStatisticsAndTradesData = true; //has to be set

	//control perzist vars (cpv)
	int &cpv_already_traded = sc.GetPersistentInt(100);
	int &cpv_already_generated = sc.GetPersistentInt(101);
	int &cpv_previous_qty = sc.GetPersistentInt(102);

	//generates the random numbers
	int &x = sc.GetPersistentInt(1);
	int &ls = sc.GetPersistentInt(3);
	if (!sc.IsFullRecalculation && !cpv_already_generated)
	{
		x = rn_generator(sc);
		ls = ls_generator();
		cpv_already_generated = 1;
	}
	const char * dir = (ls == 1) ? "Long" : "Short";

	//set the timestamps
	int curent_time = sc.BaseDateTimeIn[sc.Index].GetTime();
	SCDateTime &last_exit_dt_perzist = sc.GetPersistentSCDateTime(2);
	int exit_hour, exit_minute, exit_second;
	last_exit_dt_perzist.GetTimeHMS(exit_hour, exit_minute, exit_second);
	int last_exit_time = last_exit_dt_perzist.GetTime();
	int starting_time = rth_start.GetTime(); // at first set the starting point to start RTH
	if (cpv_already_traded != 0) { starting_time = last_exit_time; } // after each entry reset it to the timestamp of the last exit
	int time_to_entry = x + starting_time;

	s_SCNewOrder regular_order;
	regular_order.OrderType = SCT_ORDERTYPE_MARKET;
	regular_order.OrderQuantity = 1;

	s_SCPositionData current_position;
	sc.GetTradePosition(current_position);
	int position_qty{ static_cast<int>(current_position.PositionQuantity) };

	if (is_rth(sc))
	{
		if (curent_time >= time_to_entry && ls == 1)
		{
			int check_entry = static_cast<int>(sc.BuyEntry(regular_order));
			if (position_qty != 0)
			{
				cpv_previous_qty = 1; 
				x = 86400; //86400 is the number of seconds within one day, set this high number to avoid reentry in case the trade would take really long (no trade till take longer than one day)
			}
		}
		else if (curent_time >= time_to_entry && ls == 0)
		{
			int check_entry = static_cast<int>(sc.SellEntry(regular_order));
			if (position_qty != 0)
			{
				cpv_previous_qty = 1;
				x = 86400; 
			}
		}
		// after closing a possition
		if (cpv_previous_qty != 0 && position_qty == 0) 
		{
				last_exit_dt_perzist = current_position.LastExitDateTime;  
				cpv_already_traded = 1;
				cpv_previous_qty = 0;
				cpv_already_generated = 0; //set the control perzist vars to 0 to re-generate random number
		}
	}
	
	if (current_position.PositionQuantity != 0 && sc.BaseDateTimeIn[sc.Index].GetTime() >= flat_time.GetTime()) sc.FlattenAndCancelAllOrders();

	if (display_values.GetYesNo() == TRUE)
	{
		{
			s_UseTool t;
			t.Clear();
			t.ChartNumber = sc.ChartNumber;
			t.DrawingType = DRAWING_TEXT;
			t.FontSize = 10;
			t.FontBold = false;
			t.AddMethod = UTAM_ADD_OR_ADJUST;
			t.UseRelativeVerticalValues = 1;
			t.BeginDateTime = 4;
			t.BeginValue = 99;
			t.Color = RGB(255, 255, 255);
			t.Region = 0;
			t.Text.Format("Random Number: %i", x);
			t.LineNumber = 2;
			sc.UseTool(t);
		} {
			s_UseTool t;
			t.Clear();
			t.ChartNumber = sc.ChartNumber;
			t.DrawingType = DRAWING_TEXT;
			t.FontSize = 10;
			t.FontBold = false;
			t.AddMethod = UTAM_ADD_OR_ADJUST;
			t.UseRelativeVerticalValues = 1;
			t.BeginDateTime = 4;
			t.BeginValue = 97;
			t.Color = RGB(255, 255, 255);
			t.Region = 0;
			t.Text.Format("Starting Time as int: %i", starting_time);
			t.LineNumber = 5;
			sc.UseTool(t);
		} {
			s_UseTool t;
			t.Clear();
			t.ChartNumber = sc.ChartNumber;
			t.DrawingType = DRAWING_TEXT;
			t.FontSize = 10;
			t.FontBold = false;
			t.AddMethod = UTAM_ADD_OR_ADJUST;
			t.UseRelativeVerticalValues = 1;
			t.BeginDateTime = 4;
			t.BeginValue = 95;
			t.Color = RGB(255, 255, 255);
			t.Region = 0;
			t.Text.Format("Next Entry Time as int: %i", time_to_entry);
			t.LineNumber = 3;
			sc.UseTool(t);
		} {
			s_UseTool t;
			t.Clear();
			t.ChartNumber = sc.ChartNumber;
			t.DrawingType = DRAWING_TEXT;
			t.FontSize = 10;
			t.FontBold = false;
			t.AddMethod = UTAM_ADD_OR_ADJUST;
			t.UseRelativeVerticalValues = 1;
			t.BeginDateTime = 4;
			t.BeginValue = 93;
			t.Color = RGB(255, 255, 255);
			t.Region = 0;
			t.Text.Format("Current Time as int: %i", curent_time);
			t.LineNumber = 1;
			sc.UseTool(t);
		} {
			s_UseTool t;
			t.Clear();
			t.ChartNumber = sc.ChartNumber;
			t.DrawingType = DRAWING_TEXT;
			t.FontSize = 10;
			t.FontBold = false;
			t.AddMethod = UTAM_ADD_OR_ADJUST;
			t.UseRelativeVerticalValues = 1;
			t.BeginDateTime = 4;
			t.BeginValue = 91;
			t.Color = RGB(255, 255, 255);
			t.Region = 0;
			t.Text.Format("Next Entry Direction: %s", dir);
			t.LineNumber = 4;
			sc.UseTool(t);
		} {
			s_UseTool t;
			t.Clear();
			t.ChartNumber = sc.ChartNumber;
			t.DrawingType = DRAWING_TEXT;
			t.FontSize = 10;
			t.FontBold = false;
			t.AddMethod = UTAM_ADD_OR_ADJUST;
			t.UseRelativeVerticalValues = 1;
			t.BeginDateTime = 4;
			t.BeginValue = 89;
			t.Color = RGB(255, 255, 255);
			t.Region = 0;
			t.Text.Format("Last Exit Time as int: %i", last_exit_time);
			t.LineNumber = 8;
			sc.UseTool(t);
		}
	}

}
