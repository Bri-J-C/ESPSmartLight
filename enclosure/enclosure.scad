// ESPSmartLight Enclosure - Snap-fit design
// Components: ESP32-C3 Super Mini + SainSmart 2-CH Relay + HLK-PM01
// Print base and lid separately, lid snaps onto base (no screws)
//
// Measure your actual components and adjust dimensions below.

/* [Component Dimensions] */
relay_w = 51;
relay_d = 38;
relay_h = 20;
relay_mount_inset = 3;
relay_mount_dia = 3.2;

hlk_w = 34;
hlk_d = 20;
hlk_h = 15;

esp_w = 22.5;
esp_d = 18;
esp_h = 8;

/* [Enclosure] */
wall = 2.5;
floor_h = 2.5;
standoff_h = 4;
gap = 3;            // gap between components and walls
lid_lip = 2;
lid_clearance = 0.25;

/* [Mounting Tabs] */
mount_tab_w = 14;
mount_tab_l = 10;
mount_hole_dia = 4.5;

/* [Wire Openings] */
wire_w = 16;
wire_h = 13;

/* [Snap Fit] */
snap_w = 8;          // snap clip width
snap_thick = 1.5;    // clip arm thickness
snap_hook = 1;       // hook depth (how far it grabs)
snap_gap = 0.3;      // clearance

/* [Ventilation] */
vent_w = 1.5;
vent_l = 10;
vent_count = 5;
vent_gap = 3.5;

/* [Render] */
show_base = true;
show_lid = false;
explode = 0;

$fn = 40;

// --- Layout ---
right_w = max(hlk_w, esp_w);
comp_gap = 3;  // gap between HLK and ESP vertically

inner_w = gap + relay_w + gap + right_w + gap;
inner_d = max(relay_d, hlk_d + comp_gap + esp_d) + gap * 2;
inner_h = max(relay_h, hlk_h) + standoff_h + 3;

outer_w = inner_w + wall * 2;
outer_d = inner_d + wall * 2;
outer_h = inner_h + floor_h;

echo(str("Outer: ", outer_w, " x ", outer_d, " x ", outer_h + wall, " mm"));

// Component origins (inner cavity coordinates)
relay_x = gap;
relay_y = (inner_d - relay_d) / 2;

hlk_x = gap + relay_w + gap;
hlk_y = inner_d - gap - hlk_d;

esp_x = hlk_x;
esp_y = gap;

// Verify no overlaps
echo(str("Relay: X ", relay_x, "-", relay_x+relay_w,
         ", Y ", relay_y, "-", relay_y+relay_d));
echo(str("HLK:   X ", hlk_x, "-", hlk_x+hlk_w,
         ", Y ", hlk_y, "-", hlk_y+hlk_d));
echo(str("ESP:   X ", esp_x, "-", esp_x+esp_w,
         ", Y ", esp_y, "-", esp_y+esp_d));
echo(str("ESP-HLK Y gap: ", hlk_y - (esp_y + esp_d), " mm"));

// Snap clip positions - 2 on each long side (front/back walls)
snap_arm_h = lid_lip + 4;          // clip arm length (6mm)
snap_z = outer_h - snap_arm_h;     // ledge aligns with hook at bottom of arm
snap_positions_x = [outer_w * 0.3, outer_w * 0.7];

// --- Helpers ---
module rrect(w, d, h, r=3) {
    hull() for (x=[r, w-r], y=[r, d-r])
        translate([x,y,0]) cylinder(r=r, h=h);
}

module standoff(base, h, od=5.5, id=1.8) {
    // base = floor thickness to extend through; h = standoff height above floor
    difference() {
        cylinder(d=od, h=base + h);
        translate([0, 0, base + h*0.3]) cylinder(d=id, h=h*0.7 + 0.1);
    }
}

module wall_opening(w, h, depth, r=2) {
    hull() for (y=[r, w-r], z=[r, h-r])
        translate([0, y, z])
            rotate([0,90,0]) cylinder(r=r, h=depth);
}

// Snap clip on lid - projects in +Y, width along X
module snap_clip_y() {
    // Arm (extends in +Y direction, thin cantilever)
    translate([0, 0, -snap_arm_h])
        cube([snap_w, snap_thick, snap_arm_h]);
    // Hook with built-in ramp for easy insertion
    translate([0, snap_thick, -snap_arm_h])
        hull() {
            cube([snap_w, snap_hook, snap_hook + 0.5]);
            translate([0, 0, snap_hook + 0.5])
                cube([snap_w, 0.01, 1]);
        }
}

// --- Base ---
module base() {
    difference() {
        union() {
            rrect(outer_w, outer_d, outer_h);

            // Mounting tabs - left & right
            for (side = [0, 1])
                translate([side ? outer_w-1 : -mount_tab_l+1,
                           outer_d/2 - mount_tab_w/2, 0])
                    hull() {
                        cube([mount_tab_l, mount_tab_w, floor_h]);
                        translate([side ? mount_tab_l : 0, mount_tab_w/2, 0])
                            cylinder(d=mount_tab_w, h=floor_h);
                    }
            // Mounting tabs - front & back
            for (side = [0, 1])
                translate([outer_w/2 - mount_tab_w/2,
                           side ? outer_d-1 : -mount_tab_l+1, 0])
                    hull() {
                        cube([mount_tab_w, mount_tab_l, floor_h]);
                        translate([mount_tab_w/2, side ? mount_tab_l : 0, 0])
                            cylinder(d=mount_tab_w, h=floor_h);
                    }
        }

        // Hollow interior
        translate([wall, wall, floor_h])
            rrect(inner_w, inner_d, inner_h + 1, r=1.5);

        // Mount tab holes
        translate([-mount_tab_l/2, outer_d/2, -0.1])
            cylinder(d=mount_hole_dia, h=floor_h+0.2);
        translate([outer_w+mount_tab_l/2, outer_d/2, -0.1])
            cylinder(d=mount_hole_dia, h=floor_h+0.2);
        translate([outer_w/2, -mount_tab_l/2, -0.1])
            cylinder(d=mount_hole_dia, h=floor_h+0.2);
        translate([outer_w/2, outer_d+mount_tab_l/2, -0.1])
            cylinder(d=mount_hole_dia, h=floor_h+0.2);

        // Wire opening - LEFT (AC in)
        translate([-0.1, outer_d/2 - wire_w/2, floor_h + standoff_h])
            wall_opening(wire_w, wire_h, wall+0.2);

        // Wire opening - RIGHT (load out)
        translate([outer_w-wall-0.1, outer_d/2 - wire_w/2, floor_h + standoff_h])
            wall_opening(wire_w, wire_h, wall+0.2);

        // Vent slots - left/right walls (lower, mid-height)
        vent_total = vent_count * vent_l + (vent_count-1) * vent_gap;
        for (i = [0:vent_count-1]) {
            vy = (outer_d - vent_total)/2 + i*(vent_l + vent_gap);
            translate([-0.1, vy, outer_h-5]) cube([wall+0.2, vent_l, vent_w]);
            translate([outer_w-wall-0.1, vy, outer_h-5]) cube([wall+0.2, vent_l, vent_w]);
        }
        // Vent slots - front/back walls (near top, above snap zone)
        for (i = [0:3]) {
            vx = (outer_w - (4*vent_l + 3*vent_gap))/2 + i*(vent_l + vent_gap);
            translate([vx, -0.1, outer_h-3]) cube([vent_l, wall+0.2, vent_w]);
            translate([vx, outer_d-wall-0.1, outer_h-3]) cube([vent_l, wall+0.2, vent_w]);
        }

        // Snap ledge cutouts - blind pockets on inner face of front/back walls
        snap_pocket = wall * 0.6;
        for (sx = snap_positions_x) {
            // Front wall - pocket on inside face
            translate([sx - snap_w/2, wall - snap_pocket, snap_z])
                cube([snap_w, snap_pocket + 0.1, snap_hook + snap_gap]);
            // Back wall - pocket on inside face
            translate([sx - snap_w/2, outer_d - wall - 0.1, snap_z])
                cube([snap_w, snap_pocket + 0.1, snap_hook + snap_gap]);
        }

        // Strain relief zip-tie slots (through-floor cutouts)
        for (sy = [outer_d/2 - wire_w/2 - 3, outer_d/2 + wire_w/2 + 1]) {
            translate([wall + 1, sy, -0.1]) cube([3, 2, floor_h + 0.2]);
            translate([outer_w - wall - 4, sy, -0.1]) cube([3, 2, floor_h + 0.2]);
        }
    }

    // --- Interior features (extend from Z=0 through floor to avoid T-junctions) ---
    translate([wall, wall, 0]) {

        // Relay standoffs - 4 corners
        for (dx = [relay_mount_inset, relay_w - relay_mount_inset])
            for (dy = [relay_mount_inset, relay_d - relay_mount_inset])
                translate([relay_x + dx, relay_y + dy, 0])
                    standoff(floor_h, standoff_h);

        // HLK-PM01 cradle - corner posts + support rails
        translate([hlk_x, hlk_y, 0]) {
            post_h = floor_h + standoff_h + hlk_h * 0.35;
            for (dx = [-1, hlk_w + 1])
                for (dy = [-1, hlk_d + 1])
                    translate([dx, dy, 0])
                        cylinder(d=3, h=post_h);
            translate([-0.5, hlk_d/2 - 5, 0]) cube([1.5, 10, floor_h + standoff_h]);
            translate([hlk_w-1, hlk_d/2 - 5, 0]) cube([1.5, 10, floor_h + standoff_h]);
        }

        // ESP32 cradle - retained mount
        translate([esp_x, esp_y, 0]) {
            board_z = floor_h;  // board sits on floor
            rail_h = board_z + esp_h + 1;  // rails extend 1mm above board top
            lip_overhang = 1;  // inward lip to hold board down
            usb_w = 9;    // USB-C port cutout width
            usb_h = 3.5;  // USB-C port cutout height

            // Side rails
            translate([-1.2, 0, 0]) cube([1.2, esp_d, rail_h]);
            translate([esp_w, 0, 0]) cube([1.2, esp_d, rail_h]);

            // Retaining lips on top of side rails (overhang inward)
            translate([-1.2, 2, rail_h - 1])
                cube([1.2 + lip_overhang, esp_d - 4, 1]);
            translate([esp_w - lip_overhang, 2, rail_h - 1])
                cube([1.2 + lip_overhang, esp_d - 4, 1]);

            // Back stop (closed end, overlaps rails)
            translate([-1.2, -1.2, 0])
                cube([esp_w + 2.4, 1.2, board_z + esp_h]);

            // Front wall with USB-C cutout (open end)
            difference() {
                translate([-1.2, esp_d, 0])
                    cube([esp_w + 2.4, 1.2, board_z + esp_h]);
                // USB-C port opening (centered on board width)
                translate([esp_w/2 - usb_w/2, esp_d - 0.1, board_z + esp_h - usb_h - 1])
                    cube([usb_w, 1.4, usb_h + 1.1]);
            }
        }
    }
}

// --- Lid (snap-fit) ---
module lid() {
    difference() {
        union() {
            // Top plate
            rrect(outer_w, outer_d, wall);

            // Inner lip - perimeter frame
            translate([wall + lid_clearance, wall + lid_clearance, -lid_lip])
                difference() {
                    rrect(inner_w - lid_clearance*2,
                          inner_d - lid_clearance*2,
                          lid_lip, r=1);
                    translate([2, 2, -0.1])
                        rrect(inner_w - lid_clearance*2 - 4,
                              inner_d - lid_clearance*2 - 4,
                              lid_lip+0.2, r=1);
                }

            // Snap clips on front/back sides (long walls)
            for (sx = snap_positions_x) {
                // Front side clip - arm extends toward front wall (+Y → toward Y=0 wall)
                translate([sx - snap_w/2, wall + lid_clearance + snap_gap, 0])
                    mirror([0,1,0]) snap_clip_y();
                // Back side clip - arm extends toward back wall
                translate([sx - snap_w/2, outer_d - wall - lid_clearance - snap_gap, 0])
                    snap_clip_y();
            }
        }

        // Engraved label
        translate([outer_w/2, outer_d/2, wall - 0.5])
            linear_extrude(0.6)
                text("ESPSmartLight", size=5, halign="center", valign="center",
                     font="Liberation Sans:style=Bold");
    }
}

// --- Output ---
if (show_base) base();
if (show_lid) translate([0, 0, outer_h + explode]) lid();
