'use client';

import { useState } from 'react';
import { Truck, Package, PlusCircle, AlertCircle } from 'lucide-react';

interface Shipment {
  id: number;
  trackingNumber: string;
  carrier: string;
  recipient: string;
  description: string;
  status: 'Delivered' | 'Out for Delivery' | 'In Transit' | 'Ordered';
  estimatedDelivery: string;
}

export default function ShipmentsCard() {
  const [shipments, setShipments] = useState<Shipment[]>([
    {
      id: 1,
      trackingNumber: "CX123456789CA",
      carrier: "Canada Post",
      recipient: "Caleb",
      description: "Amazon Delivery",
      status: "Out for Delivery",
      estimatedDelivery: "Today, by 5:00 PM"
    },
    {
      id: 2,
      trackingNumber: "1Z999AA10123456784",
      carrier: "UPS",
      recipient: "Caleb",
      description: "Custom PCBs (JLCPCB)",
      status: "In Transit",
      estimatedDelivery: "Friday, May 29"
    }
  ]);

  const [newTracking, setNewTracking] = useState("");
  const [showAddForm, setShowAddForm] = useState(false);

  const getStatusBadge = (status: Shipment['status']) => {
    switch (status) {
      case 'Out for Delivery':
        return 'bg-amber-500/10 border-amber-500/20 text-amber-500';
      case 'In Transit':
        return 'bg-blue-500/10 border-blue-500/20 text-blue-400';
      case 'Delivered':
        return 'bg-emerald-500/10 border-emerald-500/20 text-emerald-500';
      default:
        return 'bg-zinc-800 border-zinc-700 text-zinc-400';
    }
  };

  const getStatusDot = (status: Shipment['status']) => {
    switch (status) {
      case 'Out for Delivery':
        return 'bg-amber-500';
      case 'In Transit':
        return 'bg-blue-500';
      case 'Delivered':
        return 'bg-emerald-500';
      default:
        return 'bg-zinc-650';
    }
  };

  const handleAddTracking = (e: React.FormEvent) => {
    e.preventDefault();
    if (!newTracking) return;

    // Detect carrier based on pattern or default to Canada Post
    let detectedCarrier = "Canada Post";
    if (newTracking.startsWith("1Z") || newTracking.startsWith("1z")) {
      detectedCarrier = "UPS";
    } else if (newTracking.length === 12) {
      detectedCarrier = "FedEx";
    }

    const newPkg: Shipment = {
      id: Date.now(),
      trackingNumber: newTracking.toUpperCase(),
      carrier: detectedCarrier,
      recipient: "Household",
      description: "Manual Tracking",
      status: "In Transit",
      estimatedDelivery: "Pending carrier update"
    };

    setShipments([...shipments, newPkg]);
    setNewTracking("");
    setShowAddForm(false);
  };

  const hasDeliveryToday = shipments.some(s => s.status === 'Out for Delivery');

  return (
    <div className="bg-zinc-950/50 backdrop-blur-md border border-zinc-800 p-6 rounded-3xl flex flex-col hover:border-zinc-700 transition-all duration-300 shadow-lg group relative overflow-hidden animate-flash-event">
      <div className="absolute inset-0 bg-gradient-to-br from-blue-500/[0.01] via-transparent to-transparent pointer-events-none" />

      {/* Header */}
      <div className="flex justify-between items-center mb-4">
        <h3 className="text-zinc-500 text-xs font-bold uppercase tracking-widest flex items-center gap-1.5 text-left">
          <Truck className="w-3.5 h-3.5 text-zinc-400" />
          Household Deliveries
        </h3>
        <button 
          onClick={() => setShowAddForm(!showAddForm)}
          className="text-zinc-500 hover:text-emerald-400 transition-colors"
          title="Add tracking number"
        >
          <PlusCircle className="w-4 h-4" />
        </button>
      </div>

      {/* Quick Alert Banner */}
      {hasDeliveryToday && (
        <div className="mb-4 bg-amber-500/10 border border-amber-500/20 rounded-2xl p-3.5 flex items-start gap-3 text-left">
          <AlertCircle className="w-5 h-5 text-amber-500 shrink-0 mt-0.5 animate-pulse" />
          <div>
            <h4 className="text-xs font-black text-amber-400 uppercase tracking-wider">Package Alert</h4>
            <p className="text-[11px] text-zinc-300 mt-0.5 leading-normal">
              A courier is expected at the door today. Make sure to keep the porch clear!
            </p>
          </div>
        </div>
      )}

      {/* Tracking Form */}
      {showAddForm && (
        <form onSubmit={handleAddTracking} className="mb-4 p-3 bg-zinc-900/60 border border-zinc-850 rounded-2xl animate-slide-in">
          <div className="flex gap-2">
            <input 
              type="text" 
              placeholder="Enter tracking number..."
              value={newTracking}
              onChange={(e) => setNewTracking(e.target.value)}
              className="flex-1 bg-zinc-950 border border-zinc-850 rounded-xl px-3 py-1.5 text-xs text-white focus:outline-none focus:border-zinc-700 font-mono placeholder:font-sans"
            />
            <button type="submit" className="bg-emerald-600 hover:bg-emerald-500 text-white font-bold text-xs px-3 py-1.5 rounded-xl transition-colors">
              Add
            </button>
          </div>
        </form>
      )}

      {/* Deliveries List */}
      <div className="flex flex-col gap-3">
        {shipments.map((pkg) => (
          <div key={pkg.id} className="bg-zinc-900/20 border border-zinc-850 rounded-2xl p-4 flex flex-col text-left hover:bg-zinc-900/40 transition-colors">
            <div className="flex justify-between items-start mb-2">
              <div>
                <span className="text-xs font-black text-zinc-100 tracking-tight block">
                  {pkg.description}
                </span>
                <span className="text-[10px] text-zinc-500 font-mono tracking-wider">
                  {pkg.carrier} • {pkg.trackingNumber}
                </span>
              </div>
              <span className={`flex items-center gap-1.5 px-2 py-0.5 rounded-full text-[9px] font-black uppercase tracking-wider border ${getStatusBadge(pkg.status)}`}>
                <span className={`w-1.5 h-1.5 rounded-full ${getStatusDot(pkg.status)}`}></span>
                {pkg.status}
              </span>
            </div>
            
            <div className="flex justify-between items-center mt-2 pt-2 border-t border-zinc-850 text-[10px]">
              <span className="text-zinc-500 font-bold uppercase tracking-wider">Expected Delivery</span>
              <span className="font-mono text-zinc-300 font-bold">{pkg.estimatedDelivery}</span>
            </div>
          </div>
        ))}

        {shipments.length === 0 && (
          <div className="py-6 flex flex-col items-center justify-center gap-2 border border-dashed border-zinc-850 rounded-2xl text-zinc-550">
            <Package className="w-6 h-6 text-zinc-650" />
            <span className="text-xs font-mono uppercase tracking-widest">No Active Shipments</span>
          </div>
        )}
      </div>
    </div>
  );
}
