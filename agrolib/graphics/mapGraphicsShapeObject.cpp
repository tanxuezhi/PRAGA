#include "mapGraphicsShapeObject.h"
#include "commonConstants.h"


#define MAPBORDER 10


MapGraphicsShapeObject::MapGraphicsShapeObject(MapGraphicsView* _view, MapGraphicsObject *parent) :
    MapGraphicsObject(true, parent)
{
    this->setFlag(MapGraphicsObject::ObjectIsSelectable, false);
    this->setFlag(MapGraphicsObject::ObjectIsMovable, false);
    this->setFlag(MapGraphicsObject::ObjectIsFocusable);
    view = _view;

    this->geoMap = new gis::Crit3DGeoMap();
    this->isDrawing = false;
    this->shapePointer = nullptr;
    this->nrShapes = 0;
    this->updateCenter();
}


/*!
\brief If sizeIsZoomInvariant() is true, this should return the size of the
 rectangle you want in PIXELS. If false, this should return the size of the rectangle in METERS. The
 rectangle should be centered at (0,0) regardless.
*/
QRectF MapGraphicsShapeObject::boundingRect() const
{
     return QRectF( -this->view->width() * 0.6, -this->view->height() * 0.6,
                     this->view->width() * 1.2,  this->view->height() * 1.2);
}


void MapGraphicsShapeObject::updateCenter()
{
    int widthPixels = view->width() - MAPBORDER*2;
    int heightPixels = view->height() - MAPBORDER*2;
    QPointF newCenter = view->mapToScene(QPoint(widthPixels/2, heightPixels/2));

    // reference point
    geoMap->referencePoint.latitude = newCenter.y();
    geoMap->referencePoint.longitude = newCenter.x();

    // reference pixel
    referencePixel = view->tileSource()->ll2qgs(newCenter, view->zoomLevel());

    if (isDrawing) setPos(newCenter);
}


void MapGraphicsShapeObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (this->isDrawing)
    {
        setMapExtents();

        if (this->shapePointer != nullptr)
            drawShape(painter);
    }
}


QPointF MapGraphicsShapeObject::getPixel(const LatLonPoint &geoPoint)
{
    QPointF point = QPointF(geoPoint.lon, geoPoint.lat);
    QPointF pixel = this->view->tileSource()->ll2qgs(point, this->view->zoomLevel());
    pixel.setX(pixel.x() - this->referencePixel.x());
    pixel.setY(this->referencePixel.y() - pixel.y());
    return pixel;
}


void MapGraphicsShapeObject::setPolygon(unsigned int i, unsigned int j, QPolygonF* polygon)
{
    QPointF point;

    polygon->clear();
    unsigned long offset = shapeParts[i][j].offset;
    unsigned long lenght = shapeParts[i][j].length;

    for (unsigned long v = 0; v < lenght; v++)
    {
        j = offset + v;
        point = getPixel(geoPoints[i][j]);
        polygon->append(point);
    }
}


void MapGraphicsShapeObject::drawShape(QPainter* myPainter)
{
    QPolygonF polygon;
    QPainterPath* path;
    QPainterPath* inner;

    myPainter->setPen(Qt::black);
    myPainter->setBrush(Qt::red);

    for (unsigned long i = 0; i < nrShapes; i++)
    {
        for (unsigned int j = 0; j < shapeParts[i].size(); j++)
        {
            if (shapeParts[i][j].hole)
                continue;

            if (geoBounds[i][j].v0.lon > geoMap->topRight.longitude
                    || geoBounds[i][j].v0.lat > geoMap->topRight.latitude
                    || geoBounds[i][j].v1.lon < geoMap->bottomLeft.longitude
                    || geoBounds[i][j].v1.lat < geoMap->bottomLeft.latitude)
            {
                continue;
            }

            setPolygon(i, j, &polygon);

            if (holes[i][j].size() == 0)
            {
                myPainter->drawPolygon(polygon);
            }
            else
            {
                path = new QPainterPath();
                inner = new QPainterPath();

                path->addPolygon(polygon);

                for (unsigned int k = 0; k < holes[i][j].size(); k++)
                {
                    setPolygon(i, holes[i][j][k], &polygon);
                    inner->addPolygon(polygon);
                }

                myPainter->drawPath(path->subtracted(*inner));

                delete inner;
                delete path;
            }
        }
    }
}


bool MapGraphicsShapeObject::initializeUTM(Crit3DShapeHandler* shapePtr)
{
    if (shapePtr == nullptr) return false;

    updateCenter();
    setShape(shapePtr);

    double lat, lon;
    ShapeObject myShape;
    Box<double>* bounds;
    const Point<double> *p_ptr;
    Point<double> point;

    nrShapes = unsigned(shapePointer->getShapeCount());
    shapeParts.resize(nrShapes);
    holes.resize(nrShapes);
    geoBounds.resize(nrShapes);
    geoPoints.resize(nrShapes);
    double refLatitude = geoMap->referencePoint.latitude;

    int zoneNumber = shapePtr->getUtmZone();
    if (zoneNumber < 1 || zoneNumber > 60)
        return false;

    for (unsigned int i = 0; i < nrShapes; i++)
    {
        shapePointer->getShape(int(i), myShape);
        shapeParts[i] = myShape.getParts();

        unsigned int nrParts = myShape.getPartCount();
        holes[i].resize(nrParts);
        geoBounds[i].resize(nrParts);

        for (unsigned int j = 0; j < nrParts; j++)
        {
            // bounds
            bounds = &(shapeParts[i][j].boundsPart);
            gis::utmToLatLon(zoneNumber, refLatitude, bounds->xmin, bounds->ymin, &lat, &lon);
            geoBounds[i][j].v0.lat = lat;
            geoBounds[i][j].v0.lon = lon;

            gis::utmToLatLon(zoneNumber, refLatitude, bounds->xmax, bounds->ymax, &lat, &lon);
            geoBounds[i][j].v1.lat = lat;
            geoBounds[i][j].v1.lon = lon;

            // holes
            if (shapeParts[i][j].hole)
            {
                // check first point
                unsigned long offset = shapeParts[i][j].offset;
                point = myShape.getVertex(offset);
                int index = myShape.getIndexPart(point.x, point.y);
                if (index != NODATA)
                {
                    holes[i][unsigned(index)].push_back(j);
                }
            }
        }

        // vertices
        unsigned long nrVertices = myShape.getVertexCount();
        geoPoints[i].resize(nrVertices);
        p_ptr = myShape.getVertices();
        for (unsigned long j = 0; j < nrVertices; j++)
        {
            gis::utmToLatLon(zoneNumber, refLatitude, p_ptr->x, p_ptr->y, &lat, &lon);
            geoPoints[i][j].lat = lat;
            geoPoints[i][j].lon = lon;
            p_ptr++;
        }
    }

    setDrawing(true);
    return true;
}


void MapGraphicsShapeObject::setShape(Crit3DShapeHandler* shapePtr)
{
    this->shapePointer = shapePtr;
}


Crit3DShapeHandler* MapGraphicsShapeObject::getShapePointer()
{
    return this->shapePointer;
}


void MapGraphicsShapeObject::setDrawing(bool value)
{
    this->isDrawing = value;
}


void MapGraphicsShapeObject::setMapExtents()
{
    int widthPixels = view->width() - MAPBORDER*2;
    int heightPixels = view->height() - MAPBORDER*2;
    QPointF botLeft = view->mapToScene(QPoint(0, heightPixels));
    QPointF topRight = view->mapToScene(QPoint(widthPixels, 0));

    geoMap->bottomLeft.longitude = MAXVALUE(-180, botLeft.x());
    geoMap->bottomLeft.latitude = MAXVALUE(-84, botLeft.y());
    geoMap->topRight.longitude = MINVALUE(180, topRight.x());
    geoMap->topRight.latitude = MINVALUE(84, topRight.y());
}


void MapGraphicsShapeObject::clear()
{
    setDrawing(false);

    for (unsigned int i = 0; i < nrShapes; i++)
    {
        shapeParts[i].clear();
        holes[i].clear();
        geoBounds[i].clear();
        geoPoints[i].clear();
    }
    shapeParts.clear();
    holes.clear();
    geoBounds.clear();
    geoPoints.clear();

    nrShapes = 0;
}
