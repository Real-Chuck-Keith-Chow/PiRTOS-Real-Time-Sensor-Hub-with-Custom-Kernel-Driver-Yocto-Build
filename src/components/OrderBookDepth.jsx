import React, { useMemo } from "react";
import useWebSocket from "../hooks/useWebSocket";

/**
 * OrderBookDepth
 * Expects WS messages of shape:
 * {
 *   type: "depth",
 *   bids: [[price, size], ...], // best first
 *   asks: [[price, size], ...], // best first
 *   ts:   1712345678901
 * }
 */
export default function OrderBookDepth({ depth = 10, title = "Order Book" }) {
  const { readyLabel, lastMessage } = useWebSocket();

  const { bids, asks, ts } = useMemo(() => {
    if (lastMessage?.type === "depth") return lastMessage;
    return { bids: [], asks: [], ts: null };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [lastMessage]);

  const bestBid = bids?.[0]?.[0] ?? null;
  const bestAsk = asks?.[0]?.[0] ?? null;
  const spread = bestBid != null && bestAsk != null ? (bestAsk - bestBid) : null;

  return (
    <div className="p-4 rounded-2xl shadow-md border border-gray-200">
      <div className="flex items-baseline justify-between mb-3">
        <h2 className="text-xl font-semibold">{title}</h2>
        <span className="text-xs opacity-70">WS: {readyLabel}</span>
      </div>

      <div className="grid grid-cols-2 gap-4">
        <DepthTable caption="Bids" rows={bids} depth={depth} align="right" />
        <DepthTable caption="Asks" rows={asks} depth={depth} align="left" />
      </div>

      <div className="mt-3 text-sm opacity-80 space-y-1">
        <div>Best Bid: {bestBid ?? "-"}</div>
        <div>Best Ask: {bestAsk ?? "-"}</div>
        <div>Spread: {spread ?? "-"}</div>
        <div>Last Update: {ts ? new Date(ts).toLocaleTimeString() : "-"}</div>
      </div>
    </div>
  );
}

function DepthTable({ caption, rows = [], depth = 10, align = "right" }) {
  return (
    <div>
      <h3 className="font-medium mb-1">{caption}</h3>
      <table className="w-full text-sm">
        <thead>
          <tr>
            <th className="text-left">Price</th>
            <th className={`text-${align}`}>Size</th>
          </tr>
        </thead>
        <tbody>
          {rows.slice(0, depth).map(([price, size], i) => (
            <tr key={`${caption}-${i}`} className="odd:bg-gray-50">
              <td>{price}</td>
              <td className={`text-${align}`}>{size}</td>
            </tr>
          ))}
          {rows.length === 0 && (
            <tr>
              <td colSpan={2} className="py-2 text-center opacity-60">
                Waiting for data…
              </td>
            </tr>
          )}
        </tbody>
      </table>
    </div>
  );
}
