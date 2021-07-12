// Original Author-Matthew Nichols
// Description-Create your own soft jaws and stock solid using this addin
// 
// Quick conversion of the no longer suppported JavaScript in Fusion360 to C++ by charliex nullspacelabs.com
// todo: tidy up the autos, de-duplicate some of the code that repeats, check it is useful. 

// for IsDebuggerPresent etc
#if defined(_WINDOWS) || defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <debugapi.h>
#else
#include <dlfcn.h>
#endif

#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>
#include <CAM/CAMAll.h>

using namespace adsk::core;
using namespace adsk::fusion;
using namespace adsk::cam;

Ptr<Application> app;
Ptr<UserInterface> ui;

// which system of measurement to use
static std::string _units = "";

// global error message
static Ptr<TextBoxCommandInput> _errMessage;

// check return value and display error if not ok
bool checkReturn(Ptr<Base> returnObj)
{
	if (returnObj)
		return true;
	else
		if (app && ui)
		{
			std::string errDesc;
			app->getLastError(&errDesc);
			ui->messageBox(errDesc);

			return false;
		}
		else
			return false;
}

class SoftJawsCommandInputChangedHandler : public adsk::core::InputChangedEventHandler
{
public:
	void notify(const Ptr<InputChangedEventArgs>& eventArgs) override
	{
	}
} _softJawsCommandInputChanged;

class SoftJawsCommandValidateInputsEventHandler : public adsk::core::ValidateInputsEventHandler
{
public:
	void notify(const Ptr<ValidateInputsEventArgs>& eventArgs) override
	{
		_errMessage->text("");

	}

}_softJawsCommandValidateInputs;

class SoftJawsCommandDestroyEventHandler : public adsk::core::CommandEventHandler
{
public:
	void notify(const Ptr<CommandEventArgs>& eventArgs) override
	{

		// Terminate the script since the command has finished. 
		// note: or don't when you're asking yourself why is it terminating when i told it not too...
		//adsk::terminate();
	}
} _softJawsCommandDestroy;


// Event handler for the execute event.
class SoftJawsCommandEventHandler : public adsk::core::CommandEventHandler
{
public:
	void notify(const Ptr<CommandEventArgs>& eventArgs) override
	{
		Ptr<Design> des = app->activeProduct();
		Ptr<Attributes> attribs = des->attributes();


		Ptr<Command> cmd = eventArgs->command();
		cmd->isExecutedWhenPreEmpted(false);

		Ptr<CommandInputs> inputs = cmd->commandInputs();
		if (!checkReturn(inputs))
			return;

		Ptr<DropDownCommandInput> StockType = inputs->itemById("stockselectiondrop");
		Ptr<DropDownCommandInput> FixtureSelect = inputs->itemById("PlaneSelection");
		Ptr<DropDownCommandInput> JawSelect = inputs->itemById("softJawselectiondrop");

		// values
		Ptr<ValueCommandInput> boxLengthI = inputs->itemById("boxLength"); double boxLength = boxLengthI->value();
		Ptr<ValueCommandInput> boxWidthI = inputs->itemById("boxWidth"); double boxWidth = boxWidthI->value();
		Ptr<ValueCommandInput>  boxHeightI = inputs->itemById("boxHeight"); double boxHeight = boxHeightI->value();
		Ptr<ValueCommandInput>  PartWidthI = inputs->itemById("StockWidth"); double PartWidth = PartWidthI->value();
		Ptr<ValueCommandInput>  OverlapI = inputs->itemById("GrooveWidth"); double Overlap = OverlapI->value();
		Ptr<ValueCommandInput>  GrooveDepthI = inputs->itemById("GrooveHeight"); double GrooveDepth = GrooveDepthI->value();
		double StandardWidth = 0.98;
		double StandardHeight = 1.880;
		Ptr<ValueCommandInput>  CustomwidthI = inputs->itemById("CustomJawWidth"); double Customwidth = CustomwidthI->value();
		Ptr<ValueCommandInput>  CustomheightI = inputs->itemById("CustomJawHeight"); double Customheight = CustomheightI->value();
		Ptr<ValueCommandInput>  CustomlengthI = inputs->itemById("CustomJawLength"); double Customlength = CustomlengthI->value();
		Ptr<ValueCommandInput>  CylDiaI = inputs->itemById("cylinderDia"); double CylDia = CylDiaI->value();
		Ptr<ValueCommandInput>  CylLengthI = inputs->itemById("cylinderLength"); double CylLength = CylLengthI->value();

		if (JawSelect->selectedItem()->name() == "Dove Tail") {

			Ptr<Design> design = app->activeProduct();
			if (!checkReturn(design))
			{
				ui->messageBox("A Fusion design must be active when invoking this command.");
				return;
			}

			// Get the root component of the active design.
			auto rootComp = design->rootComponent();

			// Create a new sketch on the xy plane.
			auto sketches = rootComp->sketches();
			auto yzPlane = rootComp->yZConstructionPlane();
			auto sketch = sketches->add(yzPlane);

			auto  lines = sketch->sketchCurves()->sketchLines();

			//profile for first Jaw

			auto line1 = lines->addByTwoPoints(Point3D::create(-((boxWidth / 2.0) - Overlap), 0, 0), Point3D::create(-(((boxWidth / 2.0) - Overlap + (StandardWidth * 2.54))), 0, 0));
			auto line2 = lines->addByTwoPoints(line1->endSketchPoint(), Point3D::create(-(((boxWidth / 2.0) - Overlap + (StandardWidth * 2.54))), -(StandardHeight * 2.54), 0));
			auto line3 = lines->addByTwoPoints(line2->endSketchPoint(), Point3D::create(-((boxWidth / 2.0) - Overlap), -(StandardHeight * 2.54), 0));
			auto line4 = lines->addByTwoPoints(line1->startSketchPoint(), line3->endSketchPoint());


			//Profile for second Jaw
			auto line5 = lines->addByTwoPoints(Point3D::create(((boxWidth / 2.0) - Overlap), 0, 0), Point3D::create((((boxWidth / 2.0) - Overlap + (StandardWidth * 2.54))), 0, 0));
			auto line6 = lines->addByTwoPoints(line5->endSketchPoint(), Point3D::create((((boxWidth / 2.0) - Overlap + (StandardWidth * 2.54))), -(StandardHeight * 2.54), 0));
			auto line7 = lines->addByTwoPoints(line6->endSketchPoint(), Point3D::create(((boxWidth / 2.0) - Overlap), -(StandardHeight * 2.54), 0));
			auto line8 = lines->addByTwoPoints(line5->startSketchPoint(), line7->endSketchPoint());

			auto prof = sketch->profiles()->item(0);
			auto prof2 = sketch->profiles()->item(1);

			// Create an extrusion input to be able to define the input needed for an extrusion
			// while specifying the profile and that a new component is to be created
			auto extrudes = rootComp->features()->extrudeFeatures();
			auto extInput = extrudes->createInput(prof, FeatureOperations::NewBodyFeatureOperation);
			auto extInput2 = extrudes->createInput(prof2, FeatureOperations::JoinFeatureOperation);

			// Define that the extent is a distance
			//double length = (4.0*2.54)/2
			double length = (3.9687 * 2.54) / 2.0;
			double length2 = (5.9687 * 2.54) / 2.0;
			double length3 = (7.9687 * 2.54) / 2.0;
			double length4 = (9.9687 * 2.54) / 2.0;
			auto  distance = ValueInput::createByReal(0.0);

			if (FixtureSelect->selectedItem()->name() == "4") {
				distance = ValueInput::createByReal(length);
			}
			else if (FixtureSelect->selectedItem()->name() == "6") {
				distance = ValueInput::createByReal(length2);
			}
			else if (FixtureSelect->selectedItem()->name() == "8") {
				distance = ValueInput::createByReal(length3);
			}
			else if (FixtureSelect->selectedItem()->name() == "10") {
				distance = ValueInput::createByReal(length4);
			}

			extInput->setTwoSidesDistanceExtent(distance, distance);
			extInput2->setTwoSidesDistanceExtent(distance, distance);

			// Create the extrusion.
			auto ext = extrudes->add(extInput);
			auto ext1 = extrudes->add(extInput2);


		}
		else if (JawSelect->selectedItem()->name() == "Vise") {

			if (StockType->selectedItem()->name() == "Box") {

				if (FixtureSelect->selectedItem()->name() == "YZ") {

					Ptr<Design> design = app->activeProduct();

					if (!checkReturn(design))
					{
						ui->messageBox("A Fusion design must be active when invoking this command.");
						return;
					}

					// Get the root component of the active design.
					auto rootComp = design->rootComponent();

					// Create a new sketch on the xy plane.
					auto sketches = rootComp->sketches();
					auto yzPlane = rootComp->yZConstructionPlane();
					auto sketch = sketches->add(yzPlane);


					//profile for first Jaw
					auto lines = sketch->sketchCurves()->sketchLines();
					auto line1 = lines->addByTwoPoints(Point3D::create(-((boxWidth / 2.0) - Overlap), 0, 0), Point3D::create(-(((boxWidth / 2.0) - Overlap + (Customwidth))), 0, 0));
					auto line2 = lines->addByTwoPoints(line1->endSketchPoint(), Point3D::create(-(((boxWidth / 2.0) - Overlap + (Customwidth))), -(Customheight), 0));
					auto line3 = lines->addByTwoPoints(line2->endSketchPoint(), Point3D::create(-((boxWidth / 2.0) - Overlap), -(Customheight), 0));
					auto line4 = lines->addByTwoPoints(line1->startSketchPoint(), line3->endSketchPoint());

					//Profile for second Jaw
					auto line5 = lines->addByTwoPoints(Point3D::create(((boxWidth / 2.0) - Overlap), 0, 0), Point3D::create((((boxWidth / 2.0) - Overlap + (Customwidth))), 0, 0));
					auto line6 = lines->addByTwoPoints(line5->endSketchPoint(), Point3D::create((((boxWidth / 2.0) - Overlap + (Customwidth))), -(Customheight), 0));
					auto line7 = lines->addByTwoPoints(line6->endSketchPoint(), Point3D::create(((boxWidth / 2.0) - Overlap), -(Customheight), 0));
					auto line8 = lines->addByTwoPoints(line5->startSketchPoint(), line7->endSketchPoint());

					auto prof = sketch->profiles()->item(0);
					auto prof2 = sketch->profiles()->item(1);

					// Create an extrusion input to be able to define the input needed for an extrusion
					// while specifying the profile and that a new component is to be created
					auto extrudes = rootComp->features()->extrudeFeatures();
					if (extrudes) {
						Ptr<ExtrudeFeatureInput> extInput2 = NULL;

						auto extInput = extrudes->createInput(prof, FeatureOperations::NewComponentFeatureOperation);
						assert(extInput);
						if (prof2) {
							extInput2 = extrudes->createInput(prof2, FeatureOperations::JoinFeatureOperation);
							assert(extInput2);
						}
						// Define that the extent is a distance
						//auto length = (4.0*2.54)/2
						auto length = (Customlength) / 2.0;
						auto distance = ValueInput::createByReal(length);

						// Create the extrusion.

						if (extInput) {
							extInput->setTwoSidesDistanceExtent(distance, distance);
							auto  ext = extrudes->add(extInput);
						}
						if (extInput2) {
							extInput2->setTwoSidesDistanceExtent(distance, distance);
							auto ext1 = extrudes->add(extInput2);
						}
					}

				}
				else if (FixtureSelect->selectedItem()->name() == "XZ") {

					Ptr<Design> design = app->activeProduct();
					if (!checkReturn(design))
					{
						ui->messageBox("A Fusion design must be active when invoking this command.");
						return;
					}

					// Get the root component of the active design.
					auto rootComp = design->rootComponent();

					// Create a new sketch on the xy plane.
					auto sketches = rootComp->sketches();
					auto xzPlane = rootComp->xZConstructionPlane();
					auto sketch = sketches->add(xzPlane);

					//profile for first Jaw
					auto lines = sketch->sketchCurves()->sketchLines();
					auto line1 = lines->addByTwoPoints(Point3D::create(-((boxWidth / 2.0) - Overlap), 0, 0), Point3D::create(-(((boxWidth / 2.0) - Overlap + (Customwidth))), 0, 0));
					auto line2 = lines->addByTwoPoints(line1->endSketchPoint(), Point3D::create(-(((boxWidth / 2.0) - Overlap + (Customwidth))), (Customheight), 0));
					auto line3 = lines->addByTwoPoints(line2->endSketchPoint(), Point3D::create(-((boxWidth / 2.0) - Overlap), (Customheight), 0));
					auto line4 = lines->addByTwoPoints(line1->startSketchPoint(), line3->endSketchPoint());
					//Profile for second Jaw
					auto line5 = lines->addByTwoPoints(Point3D::create(((boxWidth / 2.0) - Overlap), 0, 0), Point3D::create((((boxWidth / 2.0) - Overlap + (Customwidth))), 0, 0));
					auto line6 = lines->addByTwoPoints(line5->endSketchPoint(), Point3D::create((((boxWidth / 2.0) - Overlap + (Customwidth))), (Customheight), 0));
					auto line7 = lines->addByTwoPoints(line6->endSketchPoint(), Point3D::create(((boxWidth / 2.0) - Overlap), (Customheight), 0));
					auto line8 = lines->addByTwoPoints(line5->startSketchPoint(), line7->endSketchPoint());

					auto  prof = sketch->profiles()->item(0);
					auto prof2 = sketch->profiles()->item(1);

					// Create an extrusion input to be able to define the input needed for an extrusion
					// while specifying the profile and that a new component is to be created
					auto  extrudes = rootComp->features()->extrudeFeatures();
					auto extInput = extrudes->createInput(prof, FeatureOperations::NewComponentFeatureOperation);
					auto extInput2 = extrudes->createInput(prof2, FeatureOperations::JoinFeatureOperation);
					// Define that the extent is a distance
					//auto length = (4.0*2.54)/2
					double length = (Customlength) / 2.0;
					auto distance = ValueInput::createByReal(length);

					extInput->setTwoSidesDistanceExtent(distance, distance);
					extInput2->setTwoSidesDistanceExtent(distance, distance);
					// Create the extrusion.
					auto ext = extrudes->add(extInput);
					auto ext1 = extrudes->add(extInput2);


				}

			}
			else if (StockType->selectedItem()->name() == "Cylinder") {

				if (FixtureSelect->selectedItem()->name() == "YZ") {

					Ptr<Design> design = app->activeProduct();

					if (!checkReturn(design))
					{
						ui->messageBox("A Fusion design must be active when invoking this command.");
						return;
					}

					// Get the root component of the active design.
					auto rootComp = design->rootComponent();

					// Create a new sketch on the xy plane.
					auto sketches = rootComp->sketches();
					auto yzPlane = rootComp->yZConstructionPlane();
					auto sketch = sketches->add(yzPlane);

					//profile for first Jaw
					auto lines = sketch->sketchCurves()->sketchLines();

					auto line1 = lines->addByTwoPoints(Point3D::create(-((PartWidth / 2.0) - Overlap), 0, 0), Point3D::create(-(((PartWidth / 2.0) - Overlap + (Customwidth))), 0, 0));
					auto line2 = lines->addByTwoPoints(line1->endSketchPoint(), Point3D::create(-(((PartWidth / 2.0) - Overlap + (Customwidth))), -(Customheight), 0));
					auto line3 = lines->addByTwoPoints(line2->endSketchPoint(), Point3D::create(-((PartWidth / 2.0) - Overlap), -(Customheight), 0));
					auto line4 = lines->addByTwoPoints(line1->startSketchPoint(), line3->endSketchPoint());
					//Profile for second Jaw
					auto line5 = lines->addByTwoPoints(Point3D::create(((PartWidth / 2.0) - Overlap), 0, 0), Point3D::create((((PartWidth / 2) - Overlap + (Customwidth))), 0, 0));
					auto line6 = lines->addByTwoPoints(line5->endSketchPoint(), Point3D::create((((PartWidth / 2.0) - Overlap + (Customwidth))), -(Customheight), 0));
					auto line7 = lines->addByTwoPoints(line6->endSketchPoint(), Point3D::create(((PartWidth / 2.0) - Overlap), -(Customheight), 0));
					auto line8 = lines->addByTwoPoints(line5->startSketchPoint(), line7->endSketchPoint());

					auto prof = sketch->profiles()->item(0);
					auto prof2 = sketch->profiles()->item(1);

					// Create an extrusion input to be able to define the input needed for an extrusion
					// while specifying the profile and that a new component is to be created
					auto extrudes = rootComp->features()->extrudeFeatures();
					auto extInput = extrudes->createInput(prof, FeatureOperations::NewComponentFeatureOperation);
					auto extInput2 = extrudes->createInput(prof2, FeatureOperations::JoinFeatureOperation);
					// Define that the extent is a distance
					//auto length = (4.0*2.54)/2
					auto length = (Customlength) / 2.0;
					auto distance = ValueInput::createByReal(length);

					extInput->setTwoSidesDistanceExtent(distance, distance);
					extInput2->setTwoSidesDistanceExtent(distance, distance);
					// Create the extrusion.
					auto ext = extrudes->add(extInput);
					auto ext1 = extrudes->add(extInput2);



				}
				else if (FixtureSelect->selectedItem()->name() == "XZ") {
					Ptr<Design> design = app->activeProduct();
					if (!checkReturn(design))
					{
						ui->messageBox("A Fusion design must be active when invoking this command.");
						return;
					}

					// Get the root component of the active design.
					auto rootComp = design->rootComponent();

					// Create a new sketch on the xy plane.
					auto sketches = rootComp->sketches();
					auto xzPlane = rootComp->xZConstructionPlane();
					auto sketch = sketches->add(xzPlane);

					//profile for first Jaw
					auto lines = sketch->sketchCurves()->sketchLines();
					auto line1 = lines->addByTwoPoints(Point3D::create(-((PartWidth / 2) - Overlap), 0, 0), Point3D::create(-(((PartWidth / 2) - Overlap + (Customwidth))), 0, 0));
					auto line2 = lines->addByTwoPoints(line1->endSketchPoint(), Point3D::create(-(((PartWidth / 2) - Overlap + (Customwidth))), (Customheight), 0));
					auto line3 = lines->addByTwoPoints(line2->endSketchPoint(), Point3D::create(-((PartWidth / 2) - Overlap), (Customheight), 0));
					auto line4 = lines->addByTwoPoints(line1->startSketchPoint(), line3->endSketchPoint());
					//Profile for second Jaw
					auto line5 = lines->addByTwoPoints(Point3D::create(((PartWidth / 2) - Overlap), 0, 0), Point3D::create((((PartWidth / 2) - Overlap + (Customwidth))), 0, 0));
					auto line6 = lines->addByTwoPoints(line5->endSketchPoint(), Point3D::create((((PartWidth / 2) - Overlap + (Customwidth))), (Customheight), 0));
					auto line7 = lines->addByTwoPoints(line6->endSketchPoint(), Point3D::create(((PartWidth / 2) - Overlap), (Customheight), 0));
					auto line8 = lines->addByTwoPoints(line5->startSketchPoint(), line7->endSketchPoint());

					auto prof = sketch->profiles()->item(0);
					auto prof2 = sketch->profiles()->item(1);

					// Create an extrusion input to be able to define the input needed for an extrusion
					// while specifying the profile and that a new component is to be created
					auto extrudes = rootComp->features()->extrudeFeatures();
					auto extInput = extrudes->createInput(prof, FeatureOperations::NewComponentFeatureOperation);
					auto extInput2 = extrudes->createInput(prof2, FeatureOperations::JoinFeatureOperation);
					// Define that the extent is a distance
					//auto length = (4->0*2->54)/2
					auto length = (Customlength) / 2;
					auto distance = ValueInput::createByReal(length);

					extInput->setTwoSidesDistanceExtent(distance, distance);
					extInput2->setTwoSidesDistanceExtent(distance, distance);
					// Create the extrusion->
					auto ext = extrudes->add(extInput);
					auto ext1 = extrudes->add(extInput2);

				}
			}
			///
		}


		//Start of cutting grooves in the soft jaws
		if (StockType->selectedItem()->name() == "Box") {
			if (JawSelect->selectedItem()->name() == "Dove Tail") {
				{
					Ptr<Design> design = app->activeProduct();

					if (!checkReturn(design))
					{
						ui->messageBox("A Fusion design must be active when invoking this command.");
						return;
					}

					// Get the root component of the active design.
					auto rootComp = design->rootComponent();

					// Create a new sketch on the xy plane.
					auto sketches = rootComp->sketches();
					auto yzPlane = rootComp->yZConstructionPlane();
					auto sketch = sketches->add(yzPlane);

					//profile for first Jaw
					auto lines = sketch->sketchCurves()->sketchLines();
					auto line1 = lines->addByTwoPoints(Point3D::create(-((boxWidth / 2.0) - Overlap), 0, 0), Point3D::create(-((boxWidth / 2.0)), 0, 0));
					auto line2 = lines->addByTwoPoints(line1->endSketchPoint(), Point3D::create(-((boxWidth / 2.0)), -(GrooveDepth), 0));
					auto line3 = lines->addByTwoPoints(line2->endSketchPoint(), Point3D::create(-((boxWidth / 2.0) - Overlap), -(GrooveDepth), 0));
					auto line4 = lines->addByTwoPoints(line1->startSketchPoint(), line3->endSketchPoint());
					//Profile for second Jaw
					auto line5 = lines->addByTwoPoints(Point3D::create(((boxWidth / 2.0) - Overlap), 0, 0), Point3D::create(((boxWidth / 2.0)), 0, 0));
					auto line6 = lines->addByTwoPoints(line5->endSketchPoint(), Point3D::create(((boxWidth / 2.0)), -(GrooveDepth), 0));
					auto line7 = lines->addByTwoPoints(line6->endSketchPoint(), Point3D::create(((boxWidth / 2.0) - Overlap), -(GrooveDepth), 0));
					auto line8 = lines->addByTwoPoints(line5->startSketchPoint(), line7->endSketchPoint());

					/*auto line1 = lines->addByTwoPoints(Point3D::create(-((boxWidth/2)-Overlap), 0, 0), Point3D::create(-(((boxWidth/2)-Overlap+(StandardWidth*2.54))), 0, 0));
					auto line2 = lines->addByTwoPoints(line1->endSketchPoint(), Point3D::create(-(((boxWidth/2)-Overlap+(StandardWidth*2.54))), -(StandardHeight*2.54), 0));
					auto line3 = lines->addByTwoPoints(line2->endSketchPoint(), Point3D::create(-((boxWidth/2)-Overlap), -(StandardHeight*2.54), 0));
					auto line4 = lines->addByTwoPoints(line1->startSketchPoint(), line3->endSketchPoint());
					//Profile for second Jaw
					auto line5 = lines->addByTwoPoints(Point3D::create(((boxWidth/2)-Overlap), 0, 0), Point3D::create((((boxWidth/2)-Overlap+(StandardWidth*2.54))), 0, 0));
					auto line6 = lines->addByTwoPoints(line5->endSketchPoint(), Point3D::create((((boxWidth/2)-Overlap+(StandardWidth*2.54))), -(StandardHeight*2.54), 0));
					auto line7 = lines->addByTwoPoints(line6->endSketchPoint(), Point3D::create(((boxWidth/2)-Overlap), -(StandardHeight*2.54), 0));
					auto line8 = lines->addByTwoPoints(line5->startSketchPoint(), line7->endSketchPoint());*/

					auto prof = sketch->profiles()->item(0);
					auto prof2 = sketch->profiles()->item(1);

					// Create an extrusion input to be able to define the input needed for an extrusion
					// while specifying the profile and that a new component is to be created
					auto extrudes = rootComp->features()->extrudeFeatures();
					auto extInput = extrudes->createInput(prof, FeatureOperations::CutFeatureOperation);
					auto extInput2 = extrudes->createInput(prof2, FeatureOperations::CutFeatureOperation);
					// Define that the extent is a distance
					//auto length = (4.0*2.54)/2
					auto length = (3.9687 * 2.54) / 2;
					auto length2 = (5.9687 * 2.54) / 2;
					auto length3 = (7.9687 * 2.54) / 2;
					auto length4 = (9.9687 * 2.54) / 2;

					auto distance = ValueInput::createByReal(0.0);

					if (FixtureSelect->selectedItem()->name() == "4") {
						distance = ValueInput::createByReal(length);
					}
					else if (FixtureSelect->selectedItem()->name() == "6") {
						distance = ValueInput::createByReal(length2);
					}
					else if (FixtureSelect->selectedItem()->name() == "8") {
						distance = ValueInput::createByReal(length3);
					}
					else if (FixtureSelect->selectedItem()->name() == "10") {
						distance = ValueInput::createByReal(length4);
					}
					extInput->setTwoSidesDistanceExtent(distance, distance);
					extInput2->setTwoSidesDistanceExtent(distance, distance);
					// Create the extrusion.
					auto ext = extrudes->add(extInput);
					auto ext1 = extrudes->add(extInput2);

				}

			}
			else if (JawSelect->selectedItem()->name() == "Vise") {
				try {
					if (FixtureSelect->selectedItem()->name() == "YZ") {
						Ptr<Design> design = app->activeProduct();

						if (!checkReturn(design))
						{
							ui->messageBox("A Fusion design must be active when invoking this command.");
							return;
						}

						// Get the root component of the active design.
						auto rootComp = design->rootComponent();

						// Create a new sketch on the xy plane.
						auto sketches = rootComp->sketches();
						auto yzPlane = rootComp->yZConstructionPlane();
						auto sketch = sketches->add(yzPlane);

						//profile for first Jaw
						auto lines = sketch->sketchCurves()->sketchLines();
						auto line1 = lines->addByTwoPoints(Point3D::create(-((boxWidth / 2.0) - Overlap), 0, 0), Point3D::create(-((boxWidth / 2.0)), 0, 0));
						auto line2 = lines->addByTwoPoints(line1->endSketchPoint(), Point3D::create(-((boxWidth / 2.0)), -(GrooveDepth), 0));
						auto line3 = lines->addByTwoPoints(line2->endSketchPoint(), Point3D::create(-((boxWidth / 2.0) - Overlap), -(GrooveDepth), 0));
						auto line4 = lines->addByTwoPoints(line1->startSketchPoint(), line3->endSketchPoint());
						//Profile for second Jaw
						auto line5 = lines->addByTwoPoints(Point3D::create(((boxWidth / 2.0) - Overlap), 0, 0), Point3D::create(((boxWidth / 2.0)), 0, 0));
						auto line6 = lines->addByTwoPoints(line5->endSketchPoint(), Point3D::create(((boxWidth / 2.0)), -(GrooveDepth), 0));
						auto line7 = lines->addByTwoPoints(line6->endSketchPoint(), Point3D::create(((boxWidth / 2.0) - Overlap), -(GrooveDepth), 0));
						auto line8 = lines->addByTwoPoints(line5->startSketchPoint(), line7->endSketchPoint());

						auto prof = sketch->profiles()->item(0);
						auto prof2 = sketch->profiles()->item(1);

						// Create an extrusion input to be able to define the input needed for an extrusion
						// while specifying the profile and that a new component is to be created
						auto extrudes = rootComp->features()->extrudeFeatures();
						auto extInput = extrudes->createInput(prof, FeatureOperations::CutFeatureOperation);
						auto extInput2 = extrudes->createInput(prof2, FeatureOperations::CutFeatureOperation);
						// Define that the extent is a distance
						auto length = (Customlength) / 2;
						auto distance = ValueInput::createByReal(length);
						extInput->setTwoSidesDistanceExtent(distance, distance);
						extInput2->setTwoSidesDistanceExtent(distance, distance);
						// Create the extrusion.
						auto ext = extrudes->add(extInput);
						auto ext1 = extrudes->add(extInput2);
					}
					else if (FixtureSelect->selectedItem()->name() == "XZ") {
						Ptr<Design> design = app->activeProduct();

						if (!checkReturn(design))
						{
							ui->messageBox("A Fusion design must be active when invoking this command.");
							return;
						}

						// Get the root component of the active design.
						auto rootComp = design->rootComponent();

						// Create a new sketch on the xy plane.
						auto sketches = rootComp->sketches();
						auto xzPlane = rootComp->xZConstructionPlane();
						auto sketch = sketches->add(xzPlane);

						//profile for first Jaw
						auto lines = sketch->sketchCurves()->sketchLines();
						auto line1 = lines->addByTwoPoints(Point3D::create(-((boxWidth / 2.0) - Overlap), 0, 0), Point3D::create(-((boxWidth / 2.0)), 0, 0));
						auto line2 = lines->addByTwoPoints(line1->endSketchPoint(), Point3D::create(-((boxWidth / 2.0)), (GrooveDepth), 0));
						auto line3 = lines->addByTwoPoints(line2->endSketchPoint(), Point3D::create(-((boxWidth / 2.0) - Overlap), (GrooveDepth), 0));
						auto line4 = lines->addByTwoPoints(line1->startSketchPoint(), line3->endSketchPoint());
						//Profile for second Jaw
						auto line5 = lines->addByTwoPoints(Point3D::create(((boxWidth / 2.0) - Overlap), 0, 0), Point3D::create(((boxWidth / 2.0)), 0, 0));
						auto line6 = lines->addByTwoPoints(line5->endSketchPoint(), Point3D::create(((boxWidth / 2.0)), (GrooveDepth), 0));
						auto line7 = lines->addByTwoPoints(line6->endSketchPoint(), Point3D::create(((boxWidth / 2.0) - Overlap), (GrooveDepth), 0));
						auto line8 = lines->addByTwoPoints(line5->startSketchPoint(), line7->endSketchPoint());

						auto prof = sketch->profiles()->item(0);
						auto prof2 = sketch->profiles()->item(1);

						// Create an extrusion input to be able to define the input needed for an extrusion
						// while specifying the profile and that a new component is to be created
						auto extrudes = rootComp->features()->extrudeFeatures();
						auto extInput = extrudes->createInput(prof, FeatureOperations::CutFeatureOperation);
						auto extInput2 = extrudes->createInput(prof2, FeatureOperations::CutFeatureOperation);
						// Define that the extent is a distance
						auto length = (Customlength) / 2;
						auto distance = ValueInput::createByReal(length);
						extInput->setTwoSidesDistanceExtent(distance, distance);
						extInput2->setTwoSidesDistanceExtent(distance, distance);
						// Create the extrusion.
						auto ext = extrudes->add(extInput);
						auto ext1 = extrudes->add(extInput2);
					}

				}
				catch (...) {
				}
			}
		}
		else if (StockType->selectedItem()->name() == "Cylinder") {
			try {
				Ptr<Design> design = app->activeProduct();

				if (!checkReturn(design))
				{
					ui->messageBox("A Fusion design must be active when invoking this command.");
					return;
				}

				// Get the root component of the active design.
				auto rootComp = design->rootComponent();

				// Create a new sketch on the xy plane.
				auto sketches = rootComp->sketches();
				auto xzPlane = rootComp->xZConstructionPlane();
				auto sketch = sketches->add(xzPlane);

				// Draw some circles.
				auto circles = sketch->sketchCurves()->sketchCircles();
				auto circle1 = circles->addByCenterRadius(Point3D::create(0, 0, 0), CylDia / 2);
				//auto circle2 = circles->addByCenterRadius(Point3D::create(8, 3, 0), 3);

				// Add a circle at the center of one of the existing circles.
				//auto circle3 = circles->addByCenterRadius(circle2.centerSketchPoint, 4);
				// Get the profile defined by the circle.
				auto prof = sketch->profiles()->item(0);

				// Create an extrusion input to be able to define the input needed for an extrusion
				// while specifying the profile and that a new component is to be created
				auto extrudes = rootComp->features()->extrudeFeatures();
				auto extInput = extrudes->createInput(prof, FeatureOperations::CutFeatureOperation);

				auto distance = ValueInput::createByReal(CylLength);
				auto distance2 = ValueInput::createByReal(GrooveDepth);
				extInput->setTwoSidesDistanceExtent(distance, distance2);

				// Create the extrusion.
				auto ext = extrudes->add(extInput);
			}
			catch (...) {
			}
		}

		if (StockType->selectedItem()->name() == "Cylinder") {
			try {
				Ptr<Design> design = app->activeProduct();

				if (!checkReturn(design))
				{
					ui->messageBox("A Fusion design must be active when invoking this command.");
					return;
				}

				// Get the root component of the active design.
				auto rootComp = design->rootComponent();

				// Create a new sketch on the xy plane.
				auto sketches = rootComp->sketches();
				auto xzPlane = rootComp->xZConstructionPlane();
				auto sketch = sketches->add(xzPlane);

				// Draw some circles.
				auto circles = sketch->sketchCurves()->sketchCircles();
				auto circle1 = circles->addByCenterRadius(Point3D::create(0, 0, 0), CylDia / 2);
				//auto circle2 = circles->addByCenterRadius(Point3D::create(8, 3, 0), 3);

				// Add a circle at the center of one of the existing circles.
				//auto circle3 = circles->addByCenterRadius(circle2.centerSketchPoint, 4);
				// Get the profile defined by the circle.
				auto prof = sketch->profiles()->item(0);

				// Create an extrusion input to be able to define the input needed for an extrusion
				// while specifying the profile and that a new component is to be created
				auto extrudes = rootComp->features()->extrudeFeatures();
				auto extInput = extrudes->createInput(prof, FeatureOperations::NewBodyFeatureOperation);

				// Define that the extent is a distance extent of 5 cm.
				auto distance = ValueInput::createByReal(CylLength);
				auto distance2 = ValueInput::createByReal(GrooveDepth);
				extInput->setTwoSidesDistanceExtent(distance, distance2);

				// Create the extrusion.
				auto ext = extrudes->add(extInput);

			}
			catch (...) {
			}
		}
		else if (StockType->selectedItem()->name() == "Box") {
			try {
				Ptr<Design> design = app->activeProduct();

				if (!checkReturn(design))
				{
					ui->messageBox("A Fusion design must be active when invoking this command.");
					return;
				}

				// Get the root component of the active design.
				auto rootComp = design->rootComponent();

				// Create a new sketch on the xy plane.
				auto sketches = rootComp->sketches();
				auto xzPlane = rootComp->xZConstructionPlane();
				auto sketch = sketches->add(xzPlane);

				// Draw two connected lines.
				auto lines = sketch->sketchCurves()->sketchLines();
				// Draw a rectangle by two points.
				//auto recLines = lines->addTwoPointRectangle(Point3D::create(4, 0, 0), Point3D::create(7, 2, 0));	
				// Draw a rectangle by a center point.
				if (FixtureSelect->selectedItem()->name() == "YZ") {
					auto recLines = lines->addCenterPointRectangle(Point3D::create(0, 0, 0), Point3D::create((boxLength / 2), (boxWidth / 2.0), 0));
				}
				else if (FixtureSelect->selectedItem()->name() == "XZ") {
					auto recLines = lines->addCenterPointRectangle(Point3D::create(0, 0, 0), Point3D::create((boxWidth / 2.0), (boxLength / 2), 0));
				}
				// Draw some circles.
				//auto circles = sketch.sketchCurves.sketchCircles;
				//auto circle1 = circles->addByCenterRadius(Point3D::create(0, 0, 0), CylDia/2);
				//auto circle2 = circles->addByCenterRadius(Point3D::create(8, 3, 0), 3);

				// Add a circle at the center of one of the existing circles.
				//auto circle3 = circles->addByCenterRadius(circle2.centerSketchPoint, 4);
				// Get the profile defined by the circle.
				auto prof = sketch->profiles()->item(0);

				// Create an extrusion input to be able to define the input needed for an extrusion
				// while specifying the profile and that a new component is to be created
				auto extrudes = rootComp->features()->extrudeFeatures();
				auto extInput = extrudes->createInput(prof, FeatureOperations::NewBodyFeatureOperation);

				// Define that the extent is a distance extent of 5 cm.
				auto distance = ValueInput::createByReal(boxHeight);
				auto distance2 = ValueInput::createByReal(GrooveDepth);
				extInput->setTwoSidesDistanceExtent(distance, distance2);

				// Create the extrusion.
				auto ext = extrudes->add(extInput);

			}
			catch (...) {
			}
		}

	}
} _softJawsCommandExecute;


class SoftJawsCommandCreatedEventHandler : public adsk::core::CommandCreatedEventHandler
{
public:
	void notify(const Ptr<CommandCreatedEventArgs>& eventArgs) override
	{
		// Verify that a Fusion design is active.
		Ptr<Design> des = app->activeProduct();
		if (!checkReturn(des))
		{
			ui->messageBox("A Fusion design must be active when invoking this command.");
			return;
		}


		std::string defaultUnits = des->unitsManager()->defaultLengthUnits();

		// Determine whether to use inches or millimeters as the intial default.
		if (defaultUnits == "in" || defaultUnits == "ft")
		{
			_units = "in";
		}
		else
		{
			_units = "mm";
		}

		// Define the default values and get the previous values from the attributes.
		std::string standard;
		if (_units == "in")
		{
			standard = "English";
		}
		else
		{
			standard = "Metric";
		}

		Ptr<Attribute> standardAttrib = des->attributes()->itemByName("SoftJaws", "standard");
		if (checkReturn(standardAttrib))
			standard = standardAttrib->value();

		if (standard == "English")
		{
			_units = "in";
		}
		else
		{
			_units = "mm";
		}

		Ptr<Attributes> attribs = des->attributes();

		Ptr<Command> cmd = eventArgs->command();
		cmd->isExecutedWhenPreEmpted(false);
		Ptr<CommandInputs> inputs = cmd->commandInputs();
		if (!checkReturn(inputs))
			return;

		// create dropdown input with labelled icon  style

		Ptr<DropDownCommandInput> dropDownCommandInput_ = inputs->addDropDownCommandInput("PlaneSelection", "Sketch Plane for Soft Jaws", DropDownStyles::LabeledIconDropDownStyle);
		if (!dropDownCommandInput_)
			return;

		Ptr<ListItems> dropDownItems_ = dropDownCommandInput_->listItems();
		if (!dropDownItems_)
			return;

		dropDownItems_->add("YZ", false);
		dropDownItems_->add("XZ", true);


		Ptr<DropDownCommandInput> dropDownCommandInput1_ = inputs->addDropDownCommandInput("softJawselectiondrop", "Select Fixture Type", DropDownStyles::LabeledIconDropDownStyle);
		Ptr<ListItems> dropDownItems1_ = dropDownCommandInput1_->listItems();
		//dropDownItems1_->add("Dove Tail", false);
		dropDownItems1_->add("Vise", true);


		inputs->addValueInput("CustomJawLength", "Jaw Length.", _units, ValueInput::createByReal(0));

		inputs->addValueInput("CustomJawWidth", "Jaw Width.", _units, ValueInput::createByReal(0));

		inputs->addValueInput("CustomJawHeight", "Jaw Height.", _units, ValueInput::createByReal(0));

		inputs->addValueInput("GrooveWidth", "Groove Width. (if Stock type=Box)", _units, ValueInput::createByReal(0));

		inputs->addValueInput("GrooveHeight", "Groove Height. (For both Stock types)", _units, ValueInput::createByReal(0));


		Ptr<DropDownCommandInput>  dropDownCommandInput2_ = inputs->addDropDownCommandInput("stockselectiondrop", "Select stock type", DropDownStyles::LabeledIconDropDownStyle);
		Ptr<ListItems> dropDownItems2_ = dropDownCommandInput2_->listItems();
		dropDownItems2_->add("Box", true);
		dropDownItems2_->add("Cylinder", false);


		inputs->addValueInput("boxWidth", "Box Width", _units, ValueInput::createByReal(0));
		inputs->addValueInput("boxLength", "Box Length", _units, ValueInput::createByReal(0));
		inputs->addValueInput("boxHeight", "Box Height", _units, ValueInput::createByReal(0));
		inputs->addValueInput("cylinderDia", "Cylinder Diameter", _units, ValueInput::createByReal(0));
		inputs->addValueInput("cylinderLength", "Cylinder Length", _units, ValueInput::createByReal(0));
		inputs->addValueInput("StockWidth", "Space Between Jaws\r\n(Cylindrical stock only)", _units, ValueInput::createByReal(0));

		_errMessage = inputs->addTextBoxCommandInput("errMessage", "", "", 2, true);
		if (!checkReturn(_errMessage))
			return;

		_errMessage->isFullWidth(true);


		// Connect to the command related events.
		Ptr<InputChangedEvent> inputChangedEvent = cmd->inputChanged();
		if (!inputChangedEvent)
			return;
		bool isOk = inputChangedEvent->add(&_softJawsCommandInputChanged);
		if (!isOk)
			return;

		Ptr<ValidateInputsEvent> validateInputsEvent = cmd->validateInputs();
		if (!validateInputsEvent)
			return;
		isOk = validateInputsEvent->add(&_softJawsCommandValidateInputs);
		if (!isOk)
			return;

		Ptr<CommandEvent> executeEvent = cmd->execute();
		if (!executeEvent)
			return;
		isOk = executeEvent->add(&_softJawsCommandExecute);
		if (!isOk)
			return;

		Ptr<CommandEvent> destroyEvent = cmd->destroy();
		if (!destroyEvent)
			return;

		isOk = destroyEvent->add(&_softJawsCommandDestroy);
		if (!isOk)
			return;
	}

}_softJawsCommandCreated;


extern "C" XI_EXPORT bool run(const char* context)
{

#if defined(_WINDOWS) || defined(_WIN32) || defined(_WIN64)
	if (IsDebuggerPresent()) {
		DebugBreak();
	}
#endif

	app = Application::get();
	if (!app)
		return false;

	ui = app->userInterface();
	if (!ui)
		return false;

	// Create a command definition and add a button to the CREATE panel.
	Ptr<CommandDefinition> cmdDef = ui->commandDefinitions()->addButtonDefinition("adskSoftJawsCPPAddIn", "Soft Jaws", "Creates a set Of Soft Jaws component", "Resources/SoftJawsCAM");
	if (!checkReturn(cmdDef))
		return false;

	Ptr<ToolbarPanel> createPanel = ui->allToolbarPanels()->itemById("SolidCreatePanel");
	if (!checkReturn(createPanel))
		return false;

	Ptr<CommandControl> softJawsButton = createPanel->controls()->addCommand(cmdDef);
	if (!checkReturn(softJawsButton))
		return false;

	// Connect to the command created event.
	Ptr<CommandCreatedEvent> commandCreatedEvent = cmdDef->commandCreated();
	if (!checkReturn(commandCreatedEvent))
		return false;

	bool isOk = commandCreatedEvent->add(&_softJawsCommandCreated);
	if (!isOk)
		return false;

	std::string strContext = context;
	if (strContext.find("IsApplicationStartup", 0) != std::string::npos)
	{
		if (strContext.find("false", 0) != std::string::npos)
		{
			ui->messageBox("The \"Soft Jaws\" command has been added to the CREATE panel of the MODEL workspace.");

		}
	}

	// Prevent this module from being terminated when the script returns, because we are waiting for event handlers to fire.
	adsk::autoTerminate(false);

	return true;
}


extern "C" XI_EXPORT bool stop(const char* context)
{
	Ptr<ToolbarPanel> createPanel = ui->allToolbarPanels()->itemById("SolidCreatePanel");
	if (!checkReturn(createPanel))
		return false;

	Ptr<CommandControl> softjawsButton = createPanel->controls()->itemById("adskSoftJawsCPPAddIn");
	if (checkReturn(softjawsButton))
		softjawsButton->deleteMe();

	Ptr<CommandDefinition> cmdDef = ui->commandDefinitions()->itemById("adskSoftJawsCPPAddIn");
	if (checkReturn(cmdDef))
		cmdDef->deleteMe();

	return true;
}


#ifdef XI_WIN


BOOL APIENTRY DllMain(HMODULE hmodule, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#endif // XI_WIN
