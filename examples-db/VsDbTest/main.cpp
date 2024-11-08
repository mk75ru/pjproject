#include "VsDbTest.h"
#include <QApplication>

// -----------------------------------------------------------------------------
int main (int argc, char *argv[])
{
  QApplication app (argc, argv);
  QApplication::setStyle ("Fusion");

  VsDbTest w;
  w.show ();
  return app.exec ();
}

// -----------------------------------------------------------------------------
