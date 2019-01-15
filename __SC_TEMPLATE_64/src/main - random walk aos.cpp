#include "sierrachart.h"
#include<random>
#include<chrono>
#include<string>

SCDLLName("RANDOM WALK AOS")

unsigned int rn_generator()
{
	std::mt19937 gen;
	gen.seed(static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
	std::uniform_int_distribution<int> distribution(10, 36000); //generate numbers from 60 do 3600 (min - hour)
	return distribution(gen);
}

unsigned char ls_generator()
{
	std::mt19937 gen;
	gen.seed(static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
	std::uniform_int_distribution<int> distribution(0, 1); //generate 1/0 to decide for long/short
	return distribution(gen);
}

SCSFExport scsf_random_walk_aos(SCStudyInterfaceRef sc)
{
	SCInputRef display_values = sc.Input[0];
	SCInputRef rth_start = sc.Input[1];
	SCInputRef rth_finish = sc.Input[2];

	if (sc.SetDefaults)
	{
		rth_start.Name = "Trade From:";
		rth_start.SetTime(HMS_TIME(8, 30, 0));
		rth_finish.Name = "Trade Till:";
		rth_finish.SetTime(HMS_TIME(14, 59, 00));
		display_values.Name = "Display Values";
		display_values.SetYesNo(1);

		sc.Subgraph[0].Name = "RTH start";
		sc.Subgraph[0].DrawStyle = DRAWSTYLE_ARROW_UP;
		sc.Subgraph[0].PrimaryColor = RGB(0, 255, 0);
		sc.Subgraph[0].SecondaryColor = RGB(255, 0, 0);
		sc.Subgraph[0].SecondaryColorUsed = 1;
		sc.Subgraph[0].LineWidth = 10;

		sc.GraphName = "randow walk AOS";
		sc.AutoLoop = 1;
		sc.GraphRegion = 0;
		sc.UpdateAlways = 1;
		sc.FreeDLL = 0;

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
	int &cpv_already_traded = sc.GetPersistentInt(7);
	int &cpv_rn_already_generated = sc.GetPersistentInt(0);
	int &cpv_ls_already_generated = sc.GetPersistentInt(2);
	int &cpv_previous_qty = sc.GetPersistentInt(5);

	s_SCNewOrder regular_order;
	regular_order.OrderType = SCT_ORDERTYPE_MARKET;
	regular_order.OrderQuantity = 1;

	s_SCPositionData current_position;
	sc.GetTradePosition(current_position);
	t_OrderQuantity32_64 position_qty{ current_position.PositionQuantity };
	
	//generates the random numbers
	int &x = sc.GetPersistentInt(1);
	int &ls = sc.GetPersistentInt(3);
	if (cpv_rn_already_generated == 0 || cpv_ls_already_generated == 0)	
	{
		x = rn_generator();
		cpv_rn_already_generated = 1;
		ls = ls_generator();
		cpv_ls_already_generated = 1;
	}
	const char * dir = (ls == 1) ? "Long" : "Short";

	//set the timestamps
	int curent_time = sc.BaseDateTimeIn[sc.Index].GetTime();
	SCDateTime &last_exit_dt_perzist = sc.GetPersistentSCDateTime(2);
	int exit_hour, exit_minute, exit_second;
	last_exit_dt_perzist.GetTimeHMS(exit_hour, exit_minute, exit_second);
	int last_exit_time = last_exit_dt_perzist.GetTime(); 
	int starting_time = rth_start.GetTime(); // at first set the starting point to start RTH
	if (cpv_already_traded != 0) {	starting_time = last_exit_time;	} // after each entry reset it to the timestamp of the last exit
	int time_to_entry = x + starting_time;

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
		}{
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
		}{
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
		}{
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
		}{
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
		}{
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
		}{
			s_UseTool t;
			t.Clear();
			t.ChartNumber = sc.ChartNumber;
			t.DrawingType = DRAWING_TEXT;
			t.FontSize = 10;
			t.FontBold = false;
			t.AddMethod = UTAM_ADD_OR_ADJUST;
			t.UseRelativeVerticalValues = 1;
			t.BeginDateTime = 4;
			t.BeginValue = 87;
			t.Color = RGB(255, 255, 255);
			t.Region = 0;
			t.Text.Format("Already Traded: %i", cpv_already_traded);
			t.LineNumber = 16;
			sc.UseTool(t);
		}
	}
	//open the positon at the given time
	if (curent_time >= time_to_entry && ls == 1)
	{
		t_OrderQuantity32_64 check_entry = sc.BuyEntry(regular_order);
		if (position_qty != 0)
		{
			x = 9999999; // set x to huge number to avoid reentry
			cpv_previous_qty = static_cast<int>(current_position.PositionQuantity);
		}
	}
	else if (curent_time >= time_to_entry && ls == 0)
	{
		t_OrderQuantity32_64 check_entry = sc.SellEntry(regular_order);
		if (position_qty != 0)
		{
			x = 9999999; // set x to huge number to avoid reentry 
			cpv_previous_qty = static_cast<int>(current_position.PositionQuantity);
		}
	}
	if (cpv_previous_qty != 0 && current_position.PositionQuantity == 0) // when i close the possition
	{
			last_exit_dt_perzist = current_position.LastExitDateTime;  
			cpv_already_traded = 1;
			cpv_rn_already_generated = cpv_ls_already_generated = 0;  // set the control perzist vars to 0 to generate random number again
	}
	if (current_position.PositionQuantity != 0 && sc.BaseDateTimeIn[sc.Index].GetTime() >= rth_finish.GetTime()) sc.FlattenAndCancelAllOrders();
}
